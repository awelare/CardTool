#pragma once
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../app.h"
#include "../ui.h"
#include "numconv.h"

// Перевод чисел между системами счисления (2..36), с дробной частью
class BasesApp : public App {
public:
  void start() override { _from = 10; _to = 2; _row = 0; _buf[0] = '2'; _buf[1] = '5'; _buf[2] = '5'; _buf[3] = 0; _len = 3; _clear = true; }

  void onKey(const KeyEvent& e) override {
    switch (e.key) {
      case Key::ESC:
        if (_len > 0) { _len = 0; _buf[0] = 0; } else { requestExit(); return; }
        break;
      case Key::UP: _row = 0; break;
      case Key::DOWN: _row = 1; break;
      case Key::LEFT:  if (_row == 0) _from = _from > 2 ? _from - 1 : 36; else _to = _to > 2 ? _to - 1 : 36; break;
      case Key::RIGHT: if (_row == 0) _from = _from < 36 ? _from + 1 : 2; else _to = _to < 36 ? _to + 1 : 2; break;
      case Key::DEL: if (_len > 0) _buf[--_len] = 0; break;
      case Key::ENTER: useResult(); break;
      case Key::CHAR: typeChar(e.ch); break;
      default: break;
    }
    invalidate();
  }

  void draw() override {
    gfx.startWrite();
    if (_clear) { gfx.fillScreen(C_BG); _clear = false; }
    ui::titleBar("Системы счисления");
    char v[24];
    snprintf(v, sizeof(v), "< %d >", _from);
    ui::row(ui::LIST_TOP, "Из системы", v, _row == 0);
    snprintf(v, sizeof(v), "< %d >", _to);
    ui::row(ui::LIST_TOP + ui::ROW_H, "В систему", v, _row == 1);

    int y = ui::LIST_TOP + 2 * ui::ROW_H + 2;
    gfx.fillRect(0, y, SCREEN_W, ui::FOOT_Y - y, C_BG);
    char shown[64];
    snprintf(shown, sizeof(shown), "Число: %s_", _buf);
    gfx.setTextColor(C_FG, C_BG);
    gfx.drawString(shown, ui::PAD, y);
    y += 22;

    numconv::Num n;
    const char* er = "";
    if (_len == 0) {
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString("Введи число", ui::PAD, y);
    } else if (!numconv::parse(_buf, _from, n, &er)) {
      gfx.setTextColor(C_ERR, C_BG);
      gfx.drawString(er, ui::PAD, y);
    } else {
      line("BIN", n, 2, y);
      line("OCT", n, 8, y);
      line("DEC", n, 10, y);
      line("HEX", n, 16, y);
      char lab[8];
      snprintf(lab, sizeof(lab), "[%d]", _to);
      line(lab, n, _to, y);
    }
    ui::footer("Enter итог->ввод  Fn+, Fn+/ сист.");
    gfx.endWrite();
  }

private:
  int _from = 10, _to = 2, _row = 0;
  char _buf[40] = "";
  int _len = 0;
  bool _clear = true;

  void typeChar(char ch) {
    if (_len >= (int)sizeof(_buf) - 1) return;
    if (ch == ',') ch = '.';
    if (ch == '.') { if (strchr(_buf, '.')) return; _buf[_len++] = '.'; }
    else if (ch == '-') { if (_len == 0) _buf[_len++] = '-'; }
    else if (isalnum((unsigned char)ch)) {
      char u = (char)toupper((unsigned char)ch);
      if (numconv::digitVal(u) < _from) _buf[_len++] = u;
    }
    _buf[_len] = 0;
  }

  void useResult() {
    numconv::Num n;
    const char* er;
    if (_len == 0 || !numconv::parse(_buf, _from, n, &er)) return;
    char out[100];
    numconv::toBase(n, _to, out, sizeof(out));
    strncpy(_buf, out, sizeof(_buf) - 1);
    _buf[sizeof(_buf) - 1] = 0;
    _len = (int)strlen(_buf);
    int t = _from; _from = _to; _to = t;
  }

  // вывод строки результата с переносом (34 символа в строке)
  void line(const char* label, const numconv::Num& n, int base, int& y) {
    char out[100];
    numconv::toBase(n, base, out, sizeof(out));
    gfx.setTextColor(C_ACCENT, C_BG);
    gfx.drawString(label, ui::PAD, y);
    gfx.setTextColor(C_FG, C_BG);
    const int per = 33;
    int len = (int)strlen(out);
    char part[40];
    for (int s = 0; s < len || s == 0; s += per) {
      int k = len - s < per ? len - s : per;
      memcpy(part, out + s, k);
      part[k] = 0;
      gfx.drawString(part, ui::PAD + 44, y);
      y += 16;
      if (y > ui::FOOT_Y - 16) return;
    }
  }
};
inline App* createBases() { return new BasesApp(); }
