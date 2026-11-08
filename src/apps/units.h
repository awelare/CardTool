#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../app.h"
#include "../ui.h"
#include "units_data.h"
#include "calc_engine.h"

// Конвертер единиц: категория -> значение, "из", "в"
class UnitsApp : public App {
public:
  void start() override { _scr = CAT; _sel = 0; _first = 0; _clear = true; }

  void onKey(const KeyEvent& e) override {
    if (_scr == CAT) onCat(e); else onConv(e);
    invalidate();
  }

  void draw() override {
    gfx.startWrite();
    if (_clear) { gfx.fillScreen(C_BG); _clear = false; }
    if (_scr == CAT) drawCat(); else drawConv();
    gfx.endWrite();
  }

private:
  enum Scr : uint8_t { CAT, CONV };
  Scr _scr = CAT;
  int _sel = 0, _first = 0;
  int _cat = 0, _from = 0, _to = 1, _row = 0;
  char _buf[24] = "";
  int _len = 0;
  bool _clear = true;

  void onCat(const KeyEvent& e) {
    bool up = e.key == Key::UP   || (e.key == Key::CHAR && e.ch == ';');
    bool dn = e.key == Key::DOWN || (e.key == Key::CHAR && e.ch == '.');
    if (e.key == Key::ESC) { requestExit(); return; }
    if (up && _sel > 0) _sel--;
    if (dn && _sel < kUnitCatCount - 1) _sel++;
    if (e.key == Key::ENTER) {
      _cat = _sel; _from = 0; _to = 1;
      const UnitCat& c = kUnitCats[_cat];
      for (int i = 0; i < c.n; i++) {          // по умолчанию: из "м"/"кг"/"л"... в первую другую
        if (!strcmp(c.u[i].sym, "м") || !strcmp(c.u[i].sym, "кг") || !strcmp(c.u[i].sym, "л") ||
            !strcmp(c.u[i].sym, "м²") || !strcmp(c.u[i].sym, "с") || !strcmp(c.u[i].sym, "м/с") ||
            !strcmp(c.u[i].sym, "Па") || !strcmp(c.u[i].sym, "Дж") || !strcmp(c.u[i].sym, "Вт") ||
            !strcmp(c.u[i].sym, "Б") || !strcmp(c.u[i].sym, "°") || !strcmp(c.u[i].sym, "°C")) { _from = i; break; }
      }
      _to = (_from == 0) ? 1 : 0;
      _buf[0] = '1'; _buf[1] = 0; _len = 1;
      _row = 0; _scr = CONV; _clear = true;
    }
  }

  void onConv(const KeyEvent& e) {
    const UnitCat& c = kUnitCats[_cat];
    switch (e.key) {
      case Key::ESC: _scr = CAT; _clear = true; return;
      case Key::UP: _row = 0; return;
      case Key::DOWN: _row = 1; return;
      case Key::LEFT:
        if (_row == 0) _from = (_from + c.n - 1) % c.n; else _to = (_to + c.n - 1) % c.n;
        return;
      case Key::RIGHT:
        if (_row == 0) _from = (_from + 1) % c.n; else _to = (_to + 1) % c.n;
        return;
      case Key::ENTER: { int t = _from; _from = _to; _to = t; return; }
      case Key::DEL: if (_len > 0) _buf[--_len] = 0; return;
      case Key::CHAR: typeChar(e.ch); return;
      default: return;
    }
  }

  void typeChar(char ch) {
    if (_len >= (int)sizeof(_buf) - 1) return;
    bool hasDot = strchr(_buf, '.') != nullptr;
    bool hasE = strchr(_buf, 'e') != nullptr;
    bool hasDigit = false;
    for (int i = 0; i < _len; i++) if (isdigit((unsigned char)_buf[i])) hasDigit = true;
    if (isdigit((unsigned char)ch)) { _buf[_len++] = ch; }
    else if ((ch == '.' || ch == ',') && !hasDot && !hasE) { _buf[_len++] = '.'; }
    else if (ch == '-' && (_len == 0 || _buf[_len - 1] == 'e')) { _buf[_len++] = '-'; }
    else if ((ch == 'e' || ch == 'E') && hasDigit && !hasE) { _buf[_len++] = 'e'; }
    _buf[_len] = 0;
  }

  void drawCat() {
    int vis = ui::visibleRows();
    if (_sel < _first) _first = _sel;
    if (_sel >= _first + vis) _first = _sel - vis + 1;
    ui::titleBar("Конвертер единиц");
    char v[8];
    for (int i = 0; i < vis; i++) {
      int idx = _first + i;
      int y = ui::LIST_TOP + i * ui::ROW_H;
      if (idx < kUnitCatCount) {
        snprintf(v, sizeof(v), "%d", kUnitCats[idx].n);
        ui::row(y, kUnitCats[idx].name, v, idx == _sel);
      } else gfx.fillRect(0, y, SCREEN_W, ui::ROW_H, C_BG);
    }
    ui::footer("Fn+; Fn+. выбор  Enter вход");
  }

  void drawConv() {
    const UnitCat& c = kUnitCats[_cat];
    ui::titleBar(c.name);
    // значение
    gfx.fillRect(0, ui::LIST_TOP, SCREEN_W, ui::ROW_H, C_BG);
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString("Значение", ui::PAD, ui::LIST_TOP + 3);
    char shown[32];
    snprintf(shown, sizeof(shown), "%s_", _buf);
    gfx.setTextColor(C_FG, C_BG);
    gfx.drawString(shown, SCREEN_W - 8 - gfx.textWidth(shown), ui::LIST_TOP + 3);
    // из / в
    char v[40];
    snprintf(v, sizeof(v), "< %s >", c.u[_from].sym);
    ui::row(ui::LIST_TOP + ui::ROW_H, "Из", v, _row == 0);
    snprintf(v, sizeof(v), "< %s >", c.u[_to].sym);
    ui::row(ui::LIST_TOP + 2 * ui::ROW_H, "В", v, _row == 1);
    // полные названия
    int ny = ui::LIST_TOP + 3 * ui::ROW_H + 4;
    gfx.fillRect(0, ny, SCREEN_W, 20, C_BG);
    char nm[80];
    snprintf(nm, sizeof(nm), "%s -> %s", c.u[_from].name, c.u[_to].name);
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString(nm, ui::PAD, ny);
    // результат
    int ry = ny + 28;
    gfx.fillRect(0, ry, SCREEN_W, 80, C_BG);
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString("Результат", ui::PAD, ry);
    char res[40] = "";
    bool ok = false;
    if (_len > 0) {
      char* end;
      double x = strtod(_buf, &end);
      if (end != _buf && *end == 0) {
        double r = unitConvert(c, _from, _to, x);
        if (r == r && r - r == 0) { calc::fmt(r, res, sizeof(res)); ok = true; }
      }
    }
    if (!ok) snprintf(res, sizeof(res), "-");
    int sz = (strlen(res) <= 19) ? 2 : 1;
    gfx.setTextSize(sz);
    gfx.setTextColor(ok ? C_ACCENT : C_DIM, C_BG);
    gfx.drawString(res, ui::PAD, ry + 20);
    gfx.setTextSize(1);
    ui::footer("Enter поменять  Fn+, Fn+/ ед.  Fn+`");
  }
};
inline App* createUnits() { return new UnitsApp(); }
