#pragma once
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <SD.h>
#include "../app.h"
#include "../ui.h"
#include "../storage.h"
#include "../ir.h"
#include "../ir_file.h"
#include "../ir_presets.h"

// ИК-пульт: устройства = файлы Flipper (.ir) в папке /ir на SD, кнопки = сигналы внутри файла
class IrRemoteApp : public App {
public:
  void start() override {
    ir::begin(PIN_IR_TX);
    _scr = DEV; _sel = 0; _first = 0; _msg[0] = 0; _clear = true;
    scanDevices();
  }
  void stop() override { ir::end(); }

  void onKey(const KeyEvent& e) override {
    bool up = e.key == Key::UP   || (e.key == Key::CHAR && e.ch == ';');
    bool dn = e.key == Key::DOWN || (e.key == Key::CHAR && e.ch == '.');
    int count = (_scr == DEV) ? _ndev : _nb;
    _msg[0] = 0;
    if (e.key == Key::ESC) {
      if (_scr == BTN) { _scr = DEV; _sel = _devSel; _first = 0; _clear = true; invalidate(); }
      else requestExit();
      return;
    }
    if (up && _sel > 0) _sel--;
    if (dn && _sel < count - 1) _sel++;
    if (e.key == Key::ENTER && count > 0) {
      if (_scr == DEV) openDevice(_sel);
      else sendButton(_sel);
    }
    invalidate();
  }

  void draw() override {
    gfx.startWrite();
    if (_clear) { gfx.fillScreen(C_BG); _clear = false; }
    int vis = ui::visibleRows();
    int count = (_scr == DEV) ? _ndev : _nb;
    if (_sel < _first) _first = _sel;
    if (_sel >= _first + vis) _first = _sel - vis + 1;
    ui::titleBar(_scr == DEV ? "ИК-пульт" : _dev[_devSel]);
    if (_scr == DEV && _ndev == 0) {
      gfx.fillRect(0, ui::LIST_TOP, SCREEN_W, ui::FOOT_Y - ui::LIST_TOP, C_BG);
      gfx.setFont(UI_FONT);
      gfx.setTextColor(C_ERR, C_BG);
      gfx.drawString(_err, ui::PAD, ui::LIST_TOP + 8);
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString("Положи файлы Flipper (.ir)", ui::PAD, ui::LIST_TOP + 36);
      gfx.drawString("в папку /ir на SD-карте", ui::PAD, ui::LIST_TOP + 56);
    } else {
      for (int i = 0; i < vis; i++) {
        int idx = _first + i;
        int y = ui::LIST_TOP + i * ui::ROW_H;
        if (idx < count) {
          if (_scr == DEV) ui::row(y, _dev[idx], idx < _nBuiltin ? "встр." : "SD", idx == _sel);
          else ui::row(y, _btn[idx].name, irKindName(_btn[idx].kind), idx == _sel);
        } else {
          gfx.fillRect(0, y, SCREEN_W, ui::ROW_H, C_BG);
        }
      }
    }
    if (_msg[0]) {
      gfx.setFont(UI_FONT);
      gfx.fillRect(0, ui::FOOT_Y, SCREEN_W, 18, C_BG);
      gfx.setTextColor(_msgOk ? C_OK : C_ERR, C_BG);
      gfx.drawString(_msg, 4, ui::FOOT_Y);
    } else {
      ui::footer(_scr == DEV ? "Enter открыть  Fn+` выход" : "Enter отправить  Fn+` назад");
    }
    gfx.endWrite();
  }

private:
  enum Scr : uint8_t { DEV, BTN };
  static const int MAXDEV = 24;
  Scr _scr = DEV;
  char _dev[MAXDEV][32];
  int _ndev = 0, _devSel = 0, _nBuiltin = 0;
  IrButton _btn[IR_MAX_BTN];
  int _nb = 0;
  int _sel = 0, _first = 0;
  char _err[40] = "";
  char _msg[40] = "";
  bool _msgOk = true;
  bool _clear = true;
  char _path[56] = "";

  void scanDevices() {
    _ndev = 0; _err[0] = 0;
    for (int i = 0; i < kIrDeviceCount && _ndev < MAXDEV; i++) {   // встроенные профили
      strncpy(_dev[_ndev], kIrDevices[i].name, sizeof(_dev[0]) - 1);
      _dev[_ndev][sizeof(_dev[0]) - 1] = 0;
      _ndev++;
    }
    _nBuiltin = _ndev;
    if (!g_sdOk) return;
    File dir = SD.open("/ir");
    if (!dir || !dir.isDirectory()) return;
    while (_ndev < MAXDEV) {
      File f = dir.openNextFile();
      if (!f) break;
      if (!f.isDirectory()) {
        const char* nm = f.name();
        int len = strlen(nm);
        if (len > 3 && strcasecmp(nm + len - 3, ".ir") == 0 && len - 3 < (int)sizeof(_dev[0])) {
          char tmp[32];
          memcpy(tmp, nm, len - 3);
          tmp[len - 3] = 0;
          bool dup = false;
          for (int k = 0; k < _nBuiltin; k++) if (strcasecmp(_dev[k], tmp) == 0) dup = true;
          if (!dup) { strcpy(_dev[_ndev], tmp); _ndev++; }
        }
      }
      f.close();
    }
    dir.close();
    for (int i = _nBuiltin + 1; i < _ndev; i++) {   // сортировка файлов с SD по имени
      char t[32]; strcpy(t, _dev[i]);
      int j = i - 1;
      while (j >= _nBuiltin && strcasecmp(_dev[j], t) > 0) { strcpy(_dev[j + 1], _dev[j]); j--; }
      strcpy(_dev[j + 1], t);
    }
  }

  void openDevice(int i) {
    if (i < _nBuiltin) {                       // встроенный профиль
      const IrDevice& d = kIrDevices[i];
      _nb = d.count < IR_MAX_BTN ? d.count : IR_MAX_BTN;
      for (int k = 0; k < _nb; k++) {
        const IrPreset& p = d.buttons[k];
        IrButton& b = _btn[k];
        memset(&b, 0, sizeof(b));
        strncpy(b.name, p.name, sizeof(b.name) - 1);
        b.kind = p.kind; b.addr = p.addr; b.cmd = p.cmd;
        b.freq = 38000; b.n = p.n; b.rawp = p.raw;
      }
      _path[0] = 0;
    } else {                                   // файл с SD
      snprintf(_path, sizeof(_path), "/ir/%s.ir", _dev[i]);
      _nb = irLoadFile(_path, _btn, IR_MAX_BTN);
      if (_nb < 0) { _nb = 0; snprintf(_msg, sizeof(_msg), "Не открыть файл"); _msgOk = false; return; }
      if (_nb == 0) { snprintf(_msg, sizeof(_msg), "В файле нет кнопок"); _msgOk = false; return; }
    }
    _devSel = i; _scr = BTN; _sel = 0; _first = 0; _clear = true;
  }

  void sendButton(int i) {
    const IrButton& b = _btn[i];
    if (b.kind == IK_NONE) { snprintf(_msg, sizeof(_msg), "Протокол не поддерживается"); _msgOk = false; return; }
    if (irSendButton(_path, b)) { snprintf(_msg, sizeof(_msg), "Отправлено: %s", b.name); _msgOk = true; }
    else { snprintf(_msg, sizeof(_msg), "Ошибка отправки"); _msgOk = false; }
  }
};
inline App* createIrRemote() { return new IrRemoteApp(); }
