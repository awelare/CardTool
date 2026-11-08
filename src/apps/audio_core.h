#pragma once
// Проигрыватель звука: WAV (PCM 8/16 бит) и MP3 (декодер libhelix) -> динамик Cardputer
#include <M5Cardputer.h>
#include <SD.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include "MP3DecoderHelix.h"

using namespace libhelix;

class AudioPlayer;
static AudioPlayer* g_audioSelf = nullptr;
static void audioMp3Cb(MP3FrameInfo& info, int16_t* pcm, size_t len, void*);

class AudioPlayer {
public:
  enum Kind : uint8_t { NONE, WAV, MP3 };
  static const int CH = 0;          // канал динамика
  static const int CHUNK = 4096;    // int16-элементов в одном куске для динамика
  static const int RING = 20480;    // int16-элементов в кольцевом буфере

  ~AudioPlayer() { close(); }

  bool open(const char* path) {
    close();
    const char* dot = strrchr(path, '.');
    if (dot && !strcasecmp(dot, ".wav")) _kind = WAV;
    else if (dot && !strcasecmp(dot, ".mp3")) _kind = MP3;
    else return false;
    _file = SD.open(path, FILE_READ);
    if (!_file) { _kind = NONE; return false; }
    _ring = (int16_t*)malloc(RING * sizeof(int16_t));
    for (int i = 0; i < 3; i++) _buf[i] = (int16_t*)malloc(CHUNK * sizeof(int16_t));
    if (!_ring || !_buf[0] || !_buf[1] || !_buf[2]) { close(); return false; }
    _rd = _wr = _cnt = 0; _bi = 0; _submitted = 0;
    _eof = _done = _paused = false;
    _rate = 0; _ch = 2; _bitrate = 0; _totalMs = 0;
    bool ok = (_kind == WAV) ? openWav() : openMp3();
    if (!ok) { close(); return false; }
    _open = true;
    return true;
  }

  void close() {
    if (_open || _kind != NONE) M5Cardputer.Speaker.stop(CH);
    if (_dec) { _dec->end(); delete _dec; _dec = nullptr; }
    if (g_audioSelf == this) g_audioSelf = nullptr;
    if (_file) _file.close();
    free(_ring); _ring = nullptr;
    for (int i = 0; i < 3; i++) { free(_buf[i]); _buf[i] = nullptr; }
    _open = false; _kind = NONE;
  }

  void pause(bool p) {
    if (!_open) return;
    _paused = p;
    if (p) M5Cardputer.Speaker.stop(CH);
  }
  bool paused() const { return _paused; }
  bool isOpen() const { return _open; }
  bool finished() const { return _done; }
  Kind kind() const { return _kind; }
  uint32_t rate() const { return _rate; }
  int channels() const { return _ch; }
  int bitrateKbps() const { return _bitrate / 1000; }
  uint32_t totalMs() const { return _totalMs; }
  bool totalIsEstimate() const { return _kind == MP3; }

  uint32_t posMs() const {
    if (!_rate) return 0;
    uint64_t ms = (uint64_t)_submitted * 1000 / _rate;
    uint32_t lag = (uint32_t)((uint64_t)(CHUNK / _ch) * 1000 / _rate);
    return ms > lag ? (uint32_t)(ms - lag) : 0;
  }

  // Вызывать как можно чаще (каждый цикл)
  void pump() {
    if (!_open || _paused) return;
    fill();
    auto& sp = M5Cardputer.Speaker;
    while (_rate && sp.isPlaying(CH) < 2) {
      int avail = _cnt;
      int need = CHUNK;
      if (avail < need) {
        if (_eof && avail > 0) need = avail; else break;
      }
      need -= need % _ch;
      if (need <= 0) break;
      int16_t* b = _buf[_bi];
      _bi = (_bi + 1) % 3;
      ringRead(b, need);
      sp.playRaw(b, need, _rate, _ch == 2, 1, CH, false);
      _submitted += need / _ch;
      fill();
    }
    if (_eof && _cnt == 0 && sp.isPlaying(CH) == 0) _done = true;
  }

  // вызывается из декодера
  void onPcm(MP3FrameInfo& info, int16_t* pcm, size_t len) {
    if (!_rate) {
      _rate = (uint32_t)info.samprate;
      _ch = info.nChans >= 2 ? 2 : 1;
      _bitrate = info.bitrate;
      if (_bitrate > 0 && _endPos > _dataStart)
        _totalMs = (uint32_t)((uint64_t)(_endPos - _dataStart) * 8000ULL / (uint32_t)_bitrate);
    }
    ringWrite(pcm, (int)len);
  }

private:
  Kind _kind = NONE;
  File _file;
  bool _open = false, _eof = false, _done = false, _paused = false;
  int16_t* _ring = nullptr;
  int16_t* _buf[3] = {nullptr, nullptr, nullptr};
  int _rd = 0, _wr = 0, _cnt = 0, _bi = 0;
  uint64_t _submitted = 0;
  uint32_t _rate = 0;
  int _ch = 2, _bitrate = 0;
  uint32_t _totalMs = 0;
  // MP3
  MP3DecoderHelix* _dec = nullptr;
  uint32_t _dataStart = 0, _endPos = 0;
  // WAV
  uint32_t _wavLeft = 0;
  int _bits = 16;

  int ringFree() const { return RING - _cnt; }
  void ringWrite(const int16_t* s, int n) {
    for (int i = 0; i < n && _cnt < RING; i++) {
      _ring[_wr] = s[i];
      _wr = (_wr + 1) % RING;
      _cnt++;
    }
  }
  void ringRead(int16_t* d, int n) {
    for (int i = 0; i < n; i++) {
      d[i] = _ring[_rd];
      _rd = (_rd + 1) % RING;
    }
    _cnt -= n;
  }

  void fill() {
    if (_eof) return;
    if (_kind == WAV) { while (!_eof && ringFree() >= 2048) feedWav(); }
    else              { while (!_eof && ringFree() >= 7200) feedMp3(); }
  }

  // ---------- MP3 ----------
  bool openMp3() {
    uint32_t fsz = _file.size();
    _dataStart = 0; _endPos = fsz;
    uint8_t h[10];
    if (_file.read(h, 10) == 10 && h[0] == 'I' && h[1] == 'D' && h[2] == '3') {
      uint32_t sz = ((uint32_t)(h[6] & 0x7F) << 21) | ((uint32_t)(h[7] & 0x7F) << 14) |
                    ((uint32_t)(h[8] & 0x7F) << 7) | (uint32_t)(h[9] & 0x7F);
      _dataStart = 10 + sz + ((h[5] & 0x10) ? 10 : 0);
    }
    if (fsz > _dataStart + 128) {            // тег ID3v1 в конце файла
      uint8_t t[3];
      _file.seek(fsz - 128);
      if (_file.read(t, 3) == 3 && t[0] == 'T' && t[1] == 'A' && t[2] == 'G') _endPos = fsz - 128;
    }
    if (_dataStart >= _endPos) return false;
    _file.seek(_dataStart);
    g_audioSelf = this;
    _dec = new MP3DecoderHelix(audioMp3Cb);
    _dec->begin();
    return true;
  }

  void feedMp3() {
    uint32_t pos = _file.position();
    if (pos >= _endPos) { _eof = true; return; }
    uint8_t tmp[256];
    uint32_t want = _endPos - pos;
    if (want > sizeof(tmp)) want = sizeof(tmp);
    int n = _file.read(tmp, want);
    if (n <= 0) { _eof = true; return; }
    _dec->write(tmp, n);
  }

  // ---------- WAV ----------
  static uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
  static uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }

  bool openWav() {
    uint8_t h[12];
    if (_file.read(h, 12) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) return false;
    bool haveFmt = false;
    uint32_t dataLen = 0;
    while (true) {
      uint8_t c[8];
      if (_file.read(c, 8) != 8) return false;
      uint32_t sz = rd32(c + 4);
      if (!memcmp(c, "fmt ", 4)) {
        uint8_t f[16];
        if (sz < 16 || _file.read(f, 16) != 16) return false;
        uint16_t fmt = rd16(f);
        _ch = rd16(f + 2);
        _rate = rd32(f + 4);
        _bits = rd16(f + 14);
        if (!(fmt == 1 || fmt == 0xFFFE) || (_ch != 1 && _ch != 2) || (_bits != 8 && _bits != 16)) return false;
        haveFmt = true;
        uint32_t rest = sz - 16 + (sz & 1);
        if (rest) _file.seek(_file.position() + rest);
      } else if (!memcmp(c, "data", 4)) {
        if (!haveFmt) return false;
        dataLen = sz;
        break;
      } else {
        _file.seek(_file.position() + sz + (sz & 1));
      }
    }
    uint32_t remain = _file.size() - _file.position();
    if (dataLen == 0 || dataLen > remain) dataLen = remain;
    _wavLeft = dataLen;
    _totalMs = (uint32_t)((uint64_t)dataLen * 1000 / ((uint64_t)_rate * _ch * (_bits / 8)));
    return _rate > 0;
  }

  void feedWav() {
    if (_wavLeft == 0) { _eof = true; return; }
    uint8_t tmp[2048];
    uint32_t want = _wavLeft < sizeof(tmp) ? _wavLeft : sizeof(tmp);
    int n = _file.read(tmp, want);
    if (n <= 0) { _eof = true; return; }
    _wavLeft -= n;
    if (_bits == 16) {
      int cnt = n / 2;
      for (int i = 0; i < cnt && _cnt < RING; i++) {
        _ring[_wr] = (int16_t)(tmp[2 * i] | (tmp[2 * i + 1] << 8));
        _wr = (_wr + 1) % RING; _cnt++;
      }
    } else {
      for (int i = 0; i < n && _cnt < RING; i++) {
        _ring[_wr] = (int16_t)(((int)tmp[i] - 128) << 8);
        _wr = (_wr + 1) % RING; _cnt++;
      }
    }
  }
};

static void audioMp3Cb(MP3FrameInfo& info, int16_t* pcm, size_t len, void*) {
  if (g_audioSelf) g_audioSelf->onPcm(info, pcm, len);
}
