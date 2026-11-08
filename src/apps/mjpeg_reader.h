#pragma once
// Чтение MJPEG-потока (JPEG-кадры подряд) с SD
#include <SD.h>
#include <stdint.h>
#include <stdlib.h>

class MjpegReader {
public:
  ~MjpegReader() { close(); }

  bool open(const char* path, size_t maxFrame) {
    close();
    _f = SD.open(path, FILE_READ);
    if (!_f) return false;
    _fb = (uint8_t*)malloc(maxFrame);
    if (!_fb) { _f.close(); return false; }
    _max = maxFrame;
    _size = _f.size();
    _rp = _rn = 0;
    return true;
  }
  void close() {
    if (_f) _f.close();
    free(_fb); _fb = nullptr;
  }
  bool isOpen() const { return _fb != nullptr; }
  const uint8_t* data() const { return _fb; }

  // доля прочитанного файла 0..1
  float progress() {
    if (!_f || _size == 0) return 0;
    uint32_t pos = _f.position();
    uint32_t unread = (uint32_t)(_rn - _rp);
    return (float)(pos - (unread < pos ? unread : 0)) / (float)_size;
  }

  // Возвращает размер следующего кадра, -1 при конце файла. Слишком большие кадры пропускаются.
  int next() {
    int prev = -1;
    size_t n = 0;
    bool in = false;
    while (true) {
      if (_rp >= _rn) {
        _rn = _f.read(_rb, sizeof(_rb));
        _rp = 0;
        if (_rn <= 0) { _rn = 0; return -1; }
      }
      uint8_t b = _rb[_rp++];
      if (!in) {
        if (prev == 0xFF && b == 0xD8) { in = true; _fb[0] = 0xFF; _fb[1] = 0xD8; n = 2; }
        prev = b;
      } else {
        if (n >= _max) { in = false; prev = -1; n = 0; continue; }
        _fb[n++] = b;
        if (prev == 0xFF && b == 0xD9) return (int)n;
        prev = b;
      }
    }
  }

private:
  File _f;
  uint8_t* _fb = nullptr;
  size_t _max = 0;
  uint32_t _size = 0;
  uint8_t _rb[1024];
  int _rp = 0, _rn = 0;
};
