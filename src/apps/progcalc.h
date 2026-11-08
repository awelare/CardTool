#pragma once
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../app.h"
#include "../ui.h"
#include "prog_engine.h"

// Математика в разных системах счисления: числа в выбранной системе или 101#2, FF#16, 17#8
class ProgCalcApp : public App {
public:
  void start() override { _base = 16; _len = 0; _buf[0] = 0; _clear = true; }

  void onKey(const KeyEvent& e) override {
    switch (e.key) {
      case Key::ESC:
        if (_len > 0) { _len = 0; _buf[0] = 0; } else { requestExit(); return; }
        break;
      case Key::DEL: if (_len > 0) _buf[--_len] = 0; break;
      case Key::LEFT:  _base = _base > 2 ? _base - 1 : 36; break;
      case Key::RIGHT: _base = _base < 36 ? _base + 1 : 2; break;
      case Key::ENTER:   // быстрый перебор: 2 -> 8 -> 10 -> 16
        _base = (_base < 8) ? 8 : (_base < 10) ? 10 : (_base < 16) ? 16 : 2;
        break;
      case Key::CHAR:
        if (_len < MAXL && (isalnum((unsigned char)e.ch) || strchr(" +-*/%&|^~()<>#", e.ch))) { _buf[_len++] = e.ch; _buf[_len] = 0; }
        break;
      default: break;
    }
    invalidate();
  }

  void draw() override {
    gfx.startWrite();
    if (_clear) { gfx.fillScreen(C_BG); _clear = false; }
    ui::titleBar("Математика в СС");
    char v[24];
    snprintf(v, sizeof(v), "< %d >", _base);
    ui::row(ui::LIST_TOP, "Система по умолчанию", v, true);

    int iy = ui::LIST_TOP + ui::ROW_H + 4;
    gfx.fillRect(0, iy, SCREEN_W, 24, C_SEL);
    gfx.setTextColor(C_ACCENT, C_SEL);
    gfx.drawString(">", 6, iy + 4);
    int start = _len > 34 ? _len - 34 : 0;
    gfx.setTextColor(C_FG, C_SEL);
    gfx.drawString(_buf + start, 20, iy + 4);
    gfx.drawString("_", 20 + gfx.textWidth(_buf + start), iy + 4);

    int y = iy + 30;
    gfx.fillRect(0, y, SCREEN_W, ui::FOOT_Y - y, C_BG);
    int64_t r = 0;
    const char* er = "";
    if (_len == 0) {
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString("+ - * / % & | ^ ~ << >>", ui::PAD, y);
      gfx.drawString("and or xor not shl shr mod", ui::PAD, y + 16);
      gfx.drawString("Смешать системы: 101#2 + 17#8", ui::PAD, y + 32);
    } else if (!prog::eval(_buf, _base, r, &er)) {
      gfx.setTextColor(C_ERR, C_BG);
      gfx.drawString(er, ui::PAD, y);
    } else {
      char out[80];
      snprintf(out, sizeof(out), "%lld", (long long)r);
      row("DEC", out, y);
      prog::u64ToBase((uint64_t)r, 16, out, sizeof(out));
      row("HEX", out, y);
      prog::u64ToBase((uint64_t)r, 8, out, sizeof(out));
      row("OCT", out, y);
      prog::u64ToBase((uint64_t)r, 2, out, sizeof(out));
      row("BIN", out, y);
    }
    ui::footer("Enter 2/8/10/16  Fn+, Fn+/ сист.");
    gfx.endWrite();
  }

private:
  static const int MAXL = 60;
  int _base = 16;
  char _buf[MAXL + 1] = "";
  int _len = 0;
  bool _clear = true;

  void row(const char* label, const char* val, int& y) {
    gfx.setTextColor(C_ACCENT, C_BG);
    gfx.drawString(label, ui::PAD, y);
    gfx.setTextColor(C_FG, C_BG);
    const int per = 33;
    int len = (int)strlen(val);
    char part[40];
    for (int s = 0; s < len || s == 0; s += per) {
      int k = len - s < per ? len - s : per;
      memcpy(part, val + s, k);
      part[k] = 0;
      gfx.drawString(part, ui::PAD + 44, y);
      y += 16;
      if (y > ui::FOOT_Y - 16) return;
    }
  }
};
inline App* createProgCalc() { return new ProgCalcApp(); }
