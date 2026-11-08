#pragma once
#include <stdio.h>
#include <string.h>
#include "../app.h"
#include "../ui.h"
#include "filelist.h"
#include "audio_core.h"
#include "mjpeg_reader.h"

// Частота кадров должна совпадать с -r/fps в скрипте convert_video.bat
#ifndef VIDEO_FPS
#define VIDEO_FPS 15
#endif
#define VIDEO_MAX_FRAME (40 * 1024)

// Видеоплеер: MJPEG (.mjpeg) из папки VIDEO_DIR (по умолчанию /video) + звук из одноимённого .wav
class VideoApp : public App {
public:
  void start() override {
    _vol = 112;
    M5Cardputer.Speaker.setVolume(_vol);
    _fl.init(VIDEO_DIR);
    _scr = LIST; _err[0] = 0;
    rescan();
  }
  void stop() override { stopPlayback(); }

  void onKey(const KeyEvent& e) override {
    if (_scr == LIST) { onList(e); invalidate(); }
    else onPlay(e);
  }

  void update(uint32_t now) override {
    if (_scr != PLAY) return;
    if (_hasAudio) _ap.pump();
    if (_paused) return;
    uint32_t target = (uint32_t)((uint64_t)(now - _t0) * VIDEO_FPS / 1000);
    if (_frame > target) return;                    // впереди времени: ждём
    int len = _mj.next();
    if (len < 0) { stopPlayback(); _scr = LIST; _clear = true; invalidate(); return; }
    bool late = (_frame + 1 < target);              // отстаём больше, чем на кадр: пропускаем отрисовку
    _frame++;
    if (!late) {
      gfx.drawJpg(_mj.data(), (uint32_t)len, 0, 0);
      if (now < _ovUntil) drawOverlay(_ovText);
    }
  }

  void draw() override {
    if (_scr != LIST) return;
    gfx.startWrite();
    if (_clear) { gfx.fillScreen(C_BG); _clear = false; }
    drawList();
    gfx.endWrite();
  }

private:
  enum Scr : uint8_t { LIST, PLAY };
  Scr _scr = LIST;
  FileList _fl;
  MjpegReader _mj;
  AudioPlayer _ap;
  bool _listOk = true, _clear = true, _hasAudio = false, _paused = false;
  int _sel = 0, _first = 0, _cur = -1;
  uint8_t _vol = 112;
  uint32_t _t0 = 0, _pausedAt = 0, _frame = 0, _ovUntil = 0;
  char _ovText[32] = "";
  char _err[64] = "";

  void rescan() {
    _listOk = _fl.scan("mjpeg,mjpg");
    _sel = 0; _first = 0; _clear = true;
  }

  void onList(const KeyEvent& e) {
    bool up = e.key == Key::UP   || (e.key == Key::CHAR && e.ch == ';');
    bool dn = e.key == Key::DOWN || (e.key == Key::CHAR && e.ch == '.');
    _err[0] = 0;
    if (e.key == Key::ESC) { if (_fl.up()) rescan(); else requestExit(); return; }
    if (up && _sel > 0) _sel--;
    if (dn && _sel < _fl.n - 1) _sel++;
    if (e.key == Key::ENTER && _fl.n > 0) {
      if (_fl.it[_sel].dir) { if (_fl.enter(_sel)) rescan(); }
      else play(_sel);
    }
  }

  void onPlay(const KeyEvent& e) {
    if (e.key == Key::ESC) { stopPlayback(); _scr = LIST; _clear = true; invalidate(); return; }
    if (e.key == Key::ENTER) {
      uint32_t now = millis();
      if (!_paused) { _paused = true; _pausedAt = now - _t0; if (_hasAudio) _ap.pause(true); showOverlay("Пауза", 100000); }
      else { _paused = false; _t0 = now - _pausedAt; if (_hasAudio) _ap.pause(false); _ovUntil = 0; }
      return;
    }
    if (e.key == Key::UP && _vol < 240) { _vol += 16; M5Cardputer.Speaker.setVolume(_vol); showVol(); }
    if (e.key == Key::DOWN && _vol > 0) { _vol = _vol >= 16 ? _vol - 16 : 0; M5Cardputer.Speaker.setVolume(_vol); showVol(); }
    if (e.key == Key::RIGHT || e.key == Key::LEFT) {
      int i = _fl.nextFile(_cur, e.key == Key::RIGHT ? +1 : -1);
      if (i >= 0 && i != _cur) { stopPlayback(); play(i); }
    }
  }

  void showVol() { char b[24]; snprintf(b, sizeof(b), "Громк. %d%%", _vol * 100 / 255); showOverlay(b, 1500); }
  void showOverlay(const char* t, uint32_t ms) {
    strncpy(_ovText, t, sizeof(_ovText) - 1); _ovText[sizeof(_ovText) - 1] = 0;
    _ovUntil = millis() + ms;
    drawOverlay(_ovText);
  }
  void drawOverlay(const char* t) {
    gfx.startWrite();
    gfx.setFont(UI_FONT);
    int w = gfx.textWidth(t) + 10;
    gfx.fillRect(4, 4, w, 20, RGB565(0, 0, 0));
    gfx.setTextColor(RGB565(255, 255, 255), RGB565(0, 0, 0));
    gfx.drawString(t, 9, 6);
    gfx.endWrite();
  }

  void play(int idx) {
    char path[128];
    _fl.fullPath(idx, path, sizeof(path));
    if (!_mj.open(path, VIDEO_MAX_FRAME)) { snprintf(_err, sizeof(_err), "Не удалось открыть видео"); return; }
    // звук: одноимённый .wav
    char wav[132];
    strncpy(wav, path, sizeof(wav) - 5);
    wav[sizeof(wav) - 5] = 0;
    char* dot = strrchr(wav, '.');
    if (dot) strcpy(dot, ".wav");
    _hasAudio = SD.exists(wav) && _ap.open(wav);
    _cur = idx; _scr = PLAY; _paused = false; _frame = 0; _ovUntil = 0;
    gfx.fillScreen(RGB565(0, 0, 0));
    if (_hasAudio) _ap.pump();
    _t0 = millis();
  }

  void stopPlayback() {
    if (_hasAudio) _ap.close();
    _hasAudio = false;
    _mj.close();
    _paused = false;
  }

  void drawList() {
    int vis = ui::visibleRows();
    if (_sel < _first) _first = _sel;
    if (_sel >= _first + vis) _first = _sel - vis + 1;
    char t[48];
    if (_fl.atRoot()) snprintf(t, sizeof(t), "Видео");
    else fitText(_fl.path, SCREEN_W - 12, t, sizeof(t));
    ui::titleBar(t);
    if (!_listOk || _fl.n == 0) {
      gfx.fillRect(0, ui::LIST_TOP, SCREEN_W, ui::FOOT_Y - ui::LIST_TOP, C_BG);
      gfx.setTextColor(C_ERR, C_BG);
      gfx.drawString(!_listOk ? "Нет папки на SD:" : "В папке нет .mjpeg файлов:", ui::PAD, ui::LIST_TOP + 8);
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString(_fl.path, ui::PAD, ui::LIST_TOP + 30);
      gfx.drawString("Сконвертируй видео скриптом", ui::PAD, ui::LIST_TOP + 60);
      gfx.drawString("convert_video.bat", ui::PAD, ui::LIST_TOP + 80);
    } else {
      char lab[64], base[48];
      for (int i = 0; i < vis; i++) {
        int idx = _first + i;
        int y = ui::LIST_TOP + i * ui::ROW_H;
        if (idx < _fl.n) {
          if (_fl.it[idx].dir) snprintf(base, sizeof(base), "%s", _fl.it[idx].name);
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
    } else ui::footer("Enter смотреть  Fn+` назад");
  }
};
inline App* createVideo() { return new VideoApp(); }
