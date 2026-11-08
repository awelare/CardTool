#pragma once
#include <stdio.h>
#include <string.h>
#include "../app.h"
#include "../ui.h"
#include "calc_engine.h"

// Инженерный калькулятор: вводишь выражение, Enter = посчитать
class CalculatorApp : public App {
public:
  void start() override {
    _len = 0; _buf[0] = 0; _err[0] = 0; _hn = 0; _hi = -1;
    _ctx.deg = true; _ctx.ans = 0; _clear = true;
    updatePreview();
  }

  void onKey(const KeyEvent& e) override {
    _err[0] = 0;
    switch (e.key) {
      case Key::ESC:
        if (_len > 0) { _len = 0; _buf[0] = 0; _hi = -1; }
        else { requestExit(); return; }
        break;
      case Key::DEL:
        if (_len > 0) _buf[--_len] = 0;
        _hi = -1;
        break;
      case Key::ENTER: submit(); break;
      case Key::UP: recall(+1); break;
      case Key::DOWN: recall(-1); break;
      case Key::CHAR:
        if (e.ch == '`') _ctx.deg = !_ctx.deg;
        else if (e.ch >= 32 && e.ch < 127 && _len < MAXL) { _buf[_len++] = e.ch; _buf[_len] = 0; _hi = -1; }
        break;
      default: break;
    }
    updatePreview();
    invalidate();
  }

  void draw() override {
    gfx.startWrite();
    if (_clear) { gfx.fillScreen(C_BG); _clear = false; }
    ui::titleBar("Калькулятор");
    gfx.setTextColor(RGB565(255,255,255), C_ACCENT);
    gfx.drawString(_ctx.deg ? "DEG" : "RAD", SCREEN_W - 8 - gfx.textWidth("DEG"), 4);

    const int rows = 6;
    for (int i = 0; i < rows; i++) {
      int idx = _hn - rows + i;
      int y = ui::LIST_TOP + i * ui::ROW_H;
      if (idx >= 0) {
        char lab[MAXL + 1];
        int maxPx = SCREEN_W - 28 - gfx.textWidth(_hist[idx].res);
        shortLabel(_hist[idx].expr, maxPx, lab);
        ui::row(y, lab, _hist[idx].res, false);
        gfx.setTextColor(C_FG, C_BG);
        gfx.drawString(_hist[idx].res, SCREEN_W - 8 - gfx.textWidth(_hist[idx].res), y + 3);
      } else {
        gfx.fillRect(0, y, SCREEN_W, ui::ROW_H, C_BG);
      }
    }

    int iy = ui::LIST_TOP + rows * ui::ROW_H + 4;
    gfx.fillRect(0, iy, SCREEN_W, 24, C_SEL);
    gfx.setTextColor(C_ACCENT, C_SEL);
    gfx.drawString(">", 6, iy + 4);
    int start = _len > 34 ? _len - 34 : 0;
    gfx.setTextColor(C_FG, C_SEL);
    gfx.drawString(_buf + start, 20, iy + 4);
    gfx.drawString("_", 20 + gfx.textWidth(_buf + start), iy + 4);

    gfx.fillRect(0, iy + 26, SCREEN_W, 18, C_BG);
    if (_err[0]) {
      gfx.setTextColor(C_ERR, C_BG);
      gfx.drawString(_err, 6, iy + 26);
    } else if (_prevOk) {
      char t[40];
      snprintf(t, sizeof(t), "= %s", _prev);
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString(t, SCREEN_W - 8 - gfx.textWidth(t), iy + 26);
    }
    ui::footer("Enter = Fn+; Fn+. история ` DEG/RAD");
    gfx.endWrite();
  }

private:
  static const int MAXL = 62;
  static const int HMAX = 8;
  struct Entry { char expr[MAXL + 1]; char res[28]; };
  Entry _hist[HMAX];
  int _hn = 0, _hi = -1;
  char _buf[MAXL + 1] = "";
  int _len = 0;
  char _err[40] = "";
  char _prev[28] = "";
  bool _prevOk = false, _clear = true;
  calc::Ctx _ctx;

  static void shortLabel(const char* s, int maxPx, char* out) {
    int n = (int)strlen(s), st = 0;
    while (st < n && gfx.textWidth(s + st) > maxPx) st++;
    strcpy(out, s + st);
  }

  void updatePreview() {
    _prevOk = false;
    if (_len == 0) return;
    double v;
    const char* er;
    if (calc::eval(_buf, _ctx, v, &er)) { calc::fmt(v, _prev, sizeof(_prev)); _prevOk = true; }
  }

  void submit() {
    if (_len == 0) return;
    double v;
    const char* er;
    if (!calc::eval(_buf, _ctx, v, &er)) { strncpy(_err, er, sizeof(_err) - 1); _err[sizeof(_err) - 1] = 0; return; }
    if (_hn == HMAX) { memmove(&_hist[0], &_hist[1], sizeof(Entry) * (HMAX - 1)); _hn--; }
    strcpy(_hist[_hn].expr, _buf);
    calc::fmt(v, _hist[_hn].res, sizeof(_hist[_hn].res));
    _hn++;
    _ctx.ans = v;
    _len = 0; _buf[0] = 0; _hi = -1;
  }

  void recall(int dir) {
    if (_hn == 0) return;
    if (dir > 0) { _hi = (_hi < 0) ? 0 : (_hi + 1 < _hn ? _hi + 1 : _hi); }
    else {
      if (_hi > 0) _hi--;
      else { _hi = -1; _len = 0; _buf[0] = 0; return; }
    }
    strcpy(_buf, _hist[_hn - 1 - _hi].expr);
    _len = (int)strlen(_buf);
  }
};
inline App* createCalculator() { return new CalculatorApp(); }
