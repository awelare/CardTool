#pragma once
#include <stdio.h>
#include <string.h>
#include "../app.h"
#include "../ui.h"
#include "filelist.h"
#include "audio_core.h"

// Аудиоплеер: MP3 и WAV из папки MUSIC_DIR (по умолчанию /music) на SD
class MusicApp : public App {
public:
  void start() override {
    _vol = 112;
    M5Cardputer.Speaker.setVolume(_vol);
    _fl.init(MUSIC_DIR);
    _scr = LIST; _cur = -1; _err[0] = 0;
    rescan();
  }
  void stop() override { _ap.close(); }

  void onKey(const KeyEvent& e) override {
    if (_scr == LIST) onList(e); else onPlay(e);
    invalidate();
  }

  void update(uint32_t) override {
    if (_scr != PLAY) return;
    _ap.pump();
    if (_ap.finished()) { playNext(+1); return; }
    uint32_t s = _ap.posMs() / 1000;
    if (s != _lastSec) { _lastSec = s; invalidate(); }
  }

  void draw() override {
    gfx.startWrite();
    if (_clear) { gfx.fillScreen(C_BG); _clear = false; }
    if (_scr == LIST) drawList(); else drawPlay();
    gfx.endWrite();
  }

private:
  enum Scr : uint8_t { LIST, PLAY };
  Scr _scr = LIST;
  FileList _fl;
  AudioPlayer _ap;
  bool _listOk = true, _clear = true;
  int _sel = 0, _first = 0, _cur = -1;
  uint8_t _vol = 112;
  uint32_t _lastSec = 0xFFFFFFFF;
  char _err[64] = "";
  char _title[64] = "";

  void rescan() {
    _listOk = _fl.scan("mp3,wav");
    _sel = 0; _first = 0; _clear = true;
  }

  void onList(const KeyEvent& e) {
    bool up = e.key == Key::UP   || (e.key == Key::CHAR && e.ch == ';');
    bool dn = e.key == Key::DOWN || (e.key == Key::CHAR && e.ch == '.');
    _err[0] = 0;
    if (e.key == Key::ESC) {
      if (_fl.up()) rescan(); else requestExit();
      return;
    }
    if (up && _sel > 0) _sel--;
    if (dn && _sel < _fl.n - 1) _sel++;
    if (e.key == Key::ENTER && _fl.n > 0) {
      if (_fl.it[_sel].dir) { if (_fl.enter(_sel)) rescan(); }
      else playIdx(_sel);
    }
  }

  void onPlay(const KeyEvent& e) {
    bool up = e.key == Key::UP, dn = e.key == Key::DOWN;
    if (e.key == Key::ESC) { _ap.close(); _scr = LIST; _clear = true; return; }
    if (e.key == Key::ENTER) { _ap.pause(!_ap.paused()); return; }
    if (up && _vol < 240) { _vol += 16; M5Cardputer.Speaker.setVolume(_vol); }
    if (dn && _vol > 0)   { _vol = _vol >= 16 ? _vol - 16 : 0; M5Cardputer.Speaker.setVolume(_vol); }
    if (e.key == Key::RIGHT) playNext(+1);
    if (e.key == Key::LEFT)  playNext(-1);
  }

  void playIdx(int i) {
    char path[128];
    _fl.fullPath(i, path, sizeof(path));
    if (!_ap.open(path)) { snprintf(_err, sizeof(_err), "Не удалось открыть файл"); return; }
    _cur = i; _scr = PLAY; _clear = true; _lastSec = 0xFFFFFFFF;
    FileList::baseName(_fl.it[i].name, _title, sizeof(_title));
    M5Cardputer.Speaker.setVolume(_vol);
  }

  void playNext(int dir) {
    _ap.close();
    int i = _fl.nextFile(_cur, dir);
    if (i < 0) { _scr = LIST; _clear = true; invalidate(); return; }
    playIdx(i);
    if (!_ap.isOpen()) { _scr = LIST; _clear = true; }
    invalidate();
  }

  static void mmss(uint32_t ms, char* out, size_t cap) {
    uint32_t s = ms / 1000;
    snprintf(out, cap, "%02u:%02u", (unsigned)(s / 60), (unsigned)(s % 60));
  }

  void drawList() {
    int vis = ui::visibleRows();
    if (_sel < _first) _first = _sel;
    if (_sel >= _first + vis) _first = _sel - vis + 1;
    char t[48];
    if (_fl.atRoot()) snprintf(t, sizeof(t), "Музыка");
    else fitText(_fl.path, SCREEN_W - 12, t, sizeof(t));
    ui::titleBar(t);
    if (!_listOk || _fl.n == 0) {
      gfx.fillRect(0, ui::LIST_TOP, SCREEN_W, ui::FOOT_Y - ui::LIST_TOP, C_BG);
      gfx.setTextColor(C_ERR, C_BG);
      gfx.drawString(!_listOk ? "Нет папки на SD:" : "В папке нет MP3/WAV:", ui::PAD, ui::LIST_TOP + 8);
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString(_fl.path, ui::PAD, ui::LIST_TOP + 30);
    } else {
      char lab[64], base[48];
      for (int i = 0; i < vis; i++) {
        int idx = _first + i;
        int y = ui::LIST_TOP + i * ui::ROW_H;
        if (idx < _fl.n) {
          if (_fl.it[idx].dir) { snprintf(base, sizeof(base), "%s", _fl.it[idx].name); }
          else FileList::baseName(_fl.it[idx].name, base, sizeof(base));
          fitText(base, SCREEN_W - 90, lab, sizeof(lab));
          ui::row(y, lab, _fl.it[idx].dir ? "папка" : "", idx == _sel);
        } else gfx.fillRect(0, y, SCREEN_W, ui::ROW_H, C_BG);
      }
    }
    if (_err[0]) {
      gfx.fillRect(0, ui::FOOT_Y, SCREEN_W, 18, C_BG);
      gfx.setTextColor(C_ERR, C_BG);
      gfx.drawString(_err, 4, ui::FOOT_Y);
    } else ui::footer("Enter играть  Fn+` назад");
  }

  void drawPlay() {
    ui::titleBar("Музыка");
    char buf[96];
    // название
    gfx.fillRect(0, 30, SCREEN_W, 22, C_BG);
    fitText(_title, SCREEN_W - 2 * ui::PAD, buf, sizeof(buf));
    gfx.setTextColor(C_FG, C_BG);
    gfx.drawString(buf, ui::PAD, 32);
    // формат
    gfx.fillRect(0, 54, SCREEN_W, 20, C_BG);
    uint32_t r = _ap.rate();
    int kb = _ap.bitrateKbps();
    snprintf(buf, sizeof(buf), "%s %u.%u кГц %s",
             _ap.kind() == AudioPlayer::MP3 ? "MP3" : "WAV",
             (unsigned)(r / 1000), (unsigned)((r % 1000) / 100),
             _ap.channels() == 2 ? "стерео" : "моно");
    if (kb > 0) { size_t l = strlen(buf); snprintf(buf + l, sizeof(buf) - l, "  %d кбит/с", kb); }
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString(r ? buf : "...", ui::PAD, 56);
    // время
    gfx.fillRect(0, 86, SCREEN_W, 36, C_BG);
    char tm[16];
    mmss(_ap.posMs(), tm, sizeof(tm));
    gfx.setTextSize(2);
    gfx.setTextColor(C_ACCENT, C_BG);
    gfx.drawString(tm, ui::PAD, 90);
    gfx.setTextSize(1);
    if (_ap.totalMs() > 0) {
      char tt[24];
      mmss(_ap.totalMs(), tm, sizeof(tm));
      snprintf(tt, sizeof(tt), "/ %s%s", _ap.totalIsEstimate() ? "~" : "", tm);
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString(tt, 100, 98);
    }
    // прогресс
    const int bx = ui::PAD, bw = SCREEN_W - 2 * ui::PAD;
    gfx.fillRect(bx, 130, bw, 10, C_SEL);
    if (_ap.totalMs() > 0) {
      uint32_t p = _ap.posMs();
      if (p > _ap.totalMs()) p = _ap.totalMs();
      gfx.fillRect(bx, 130, (int)((uint64_t)bw * p / _ap.totalMs()), 10, C_ACCENT);
    }
    // громкость
    gfx.fillRect(0, 152, SCREEN_W, 20, C_BG);
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString("Громкость", ui::PAD, 154);
    gfx.fillRect(110, 158, 150, 8, C_SEL);
    gfx.fillRect(110, 158, (int)(150 * _vol / 255), 8, C_FG);
    // состояние и номер трека
    gfx.fillRect(0, 180, SCREEN_W, 22, C_BG);
    gfx.setTextColor(_ap.paused() ? C_ERR : C_OK, C_BG);
    gfx.drawString(_ap.paused() ? "Пауза" : "Играет", ui::PAD, 182);
    int cnt = 0, num = 0;
    for (int i = 0; i < _fl.n; i++) if (!_fl.it[i].dir) { cnt++; if (i == _cur) num = cnt; }
    snprintf(buf, sizeof(buf), "Трек %d / %d", num, cnt);
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString(buf, SCREEN_W - 10 - gfx.textWidth(buf), 182);
    ui::footer("Enter пауза Fn+,/ трек Fn+;. гр.");
  }
};
inline App* createMusic() { return new MusicApp(); }
