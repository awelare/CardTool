#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../app.h"
#include "../ui.h"
#include "elements_data.h"
#include "richtext.h"

// Периодическая система (короткопериодная форма). Курсор по таблице, Enter = карточка элемента.
class PeriodicApp : public App {
public:
  void start() override {
    memset(_cell, 0, sizeof(_cell));
    buildLayout();
    _cur = 26; _prev = 0; _scr = TABLE; _full = true; _scroll = 0;
  }

  void onKey(const KeyEvent& e) override {
    bool up = e.key == Key::UP    || (e.key == Key::CHAR && e.ch == ';');
    bool dn = e.key == Key::DOWN  || (e.key == Key::CHAR && e.ch == '.');
    bool lf = e.key == Key::LEFT  || (e.key == Key::CHAR && e.ch == ',');
    bool rt = e.key == Key::RIGHT || (e.key == Key::CHAR && e.ch == '/');
    if (_scr == TABLE) {
      if (e.key == Key::ESC) { requestExit(); return; }
      int z = _cur;
      if (up) z = moveUD(_cur, -1);
      if (dn) z = moveUD(_cur, +1);
      if (lf) z = moveLR(_cur, -1);
      if (rt) z = moveLR(_cur, +1);
      if (z != _cur) { _prev = _cur; _cur = z; invalidate(); }
      if (e.key == Key::ENTER) { _scr = DETAIL; _scroll = 0; _full = true; invalidate(); }
    } else {
      if (e.key == Key::ESC) { _scr = TABLE; _full = true; _prev = 0; invalidate(); return; }
      if (lf && _cur > 1)   { _cur--; _scroll = 0; invalidate(); }
      if (rt && _cur < 118) { _cur++; _scroll = 0; invalidate(); }
      if (up && _scroll > 0) { _scroll -= 16; if (_scroll < 0) _scroll = 0; invalidate(); }
      if (dn && _scroll < _maxScroll) { _scroll += 16; invalidate(); }
    }
  }

  void draw() override {
    gfx.startWrite();
    if (_scr == TABLE) drawTable(); else drawDetail();
    gfx.endWrite();
  }

private:
  enum Scr : uint8_t { TABLE, DETAIL };
  struct Cell { int8_t row, col; };
  static const int ROWS = 13, HDR = 11, CH = 16, LM = 10;
  Scr _scr = TABLE;
  Cell _cell[119];
  int _cur = 26, _prev = 0;
  bool _full = true;
  int _scroll = 0, _maxScroll = 0;

  void put(int z, int r, int c) { _cell[z].row = (int8_t)r; _cell[z].col = (int8_t)c; }

  void buildLayout() {
    put(1, 0, 0); put(2, 0, 9);
    for (int i = 0; i < 7; i++) { put(3 + i, 1, i); put(11 + i, 2, i); }
    put(10, 1, 9); put(18, 2, 9);
    for (int i = 0; i < 10; i++) put(19 + i, 3, i);
    for (int i = 0; i < 7; i++) put(29 + i, 4, i);
    put(36, 4, 9);
    for (int i = 0; i < 10; i++) put(37 + i, 5, i);
    for (int i = 0; i < 7; i++) put(47 + i, 6, i);
    put(54, 6, 9);
    put(55, 7, 0); put(56, 7, 1); put(57, 7, 2);
    for (int i = 0; i < 7; i++) put(72 + i, 7, 3 + i);
    for (int i = 0; i < 7; i++) put(79 + i, 8, i);
    put(86, 8, 9);
    put(87, 9, 0); put(88, 9, 1); put(89, 9, 2);
    for (int i = 0; i < 7; i++) put(104 + i, 9, 3 + i);
    for (int i = 0; i < 7; i++) put(111 + i, 10, i);
    put(118, 10, 9);
    for (int i = 0; i < 14; i++) { put(58 + i, 11, i); put(90 + i, 12, i); }
  }

  void geom(int z, int& x, int& y, int& w) const {
    int r = _cell[z].row, c = _cell[z].col;
    y = HDR + r * CH;
    if (r < 11) { x = LM + c * 30; w = 29; }
    else        { x = LM + c * 21; w = 20; }
  }
  int centerX(int z) const { int x, y, w; geom(z, x, y, w); return x + w / 2; }

  int moveLR(int z, int dir) const {
    int cx = centerX(z), best = z, bd = 9999;
    for (int k = 1; k <= 118; k++) {
      if (_cell[k].row != _cell[z].row || k == z) continue;
      int dx = centerX(k) - cx;
      if ((dir > 0 && dx > 0) || (dir < 0 && dx < 0)) { int a = abs(dx); if (a < bd) { bd = a; best = k; } }
    }
    return best;
  }
  int moveUD(int z, int dir) const {
    int cx = centerX(z);
    for (int r = _cell[z].row + dir; r >= 0 && r < ROWS; r += dir) {
      int best = 0, bd = 9999;
      for (int k = 1; k <= 118; k++) {
        if (_cell[k].row != r) continue;
        int a = abs(centerX(k) - cx);
        if (a < bd) { bd = a; best = k; }
      }
      if (best) return best;
    }
    return z;
  }

  static uint16_t catColor(int cat) {
    static const uint16_t c[10] = {
      RGB565(255,105,105), RGB565(255,170,90), RGB565(120,170,255), RGB565(255,225,110), RGB565(215,150,255),
      RGB565(110,205,175), RGB565(185,215,110), RGB565(110,215,110), RGB565(255,150,200), RGB565(100,225,240)};
    return c[cat];
  }

  void drawCell(int z, bool sel) {
    int x, y, w;
    geom(z, x, y, w);
    const Element& e = kElements[z - 1];
    uint16_t col = catColor(e.cat);
    gfx.setClipRect(x, y, w, CH - 1);
    gfx.fillRect(x, y, w, CH - 1, sel ? RGB565(0, 0, 0) : col);
    gfx.setFont(UI_FONT);
    gfx.setTextColor(sel ? RGB565(255, 255, 255) : RGB565(0, 0, 0));
    int tw = gfx.textWidth(e.sym);
    gfx.drawString(e.sym, x + (w - tw) / 2, y);
    if (sel) { gfx.drawRect(x, y, w, CH - 1, col); gfx.drawRect(x + 1, y + 1, w - 2, CH - 3, col); }
    gfx.clearClipRect();
  }

  void drawTable() {
    if (_full) {
      gfx.fillScreen(C_BG);
      gfx.setFont(UI_FONT_SMALL);
      gfx.setTextColor(C_DIM);
      static const char* g[8] = {"I", "II", "III", "IV", "V", "VI", "VII", "VIII"};
      for (int i = 0; i < 7; i++) {
        int cx = LM + i * 30 + 14;
        gfx.drawString(g[i], cx - gfx.textWidth(g[i]) / 2, 0);
      }
      int cx = LM + 8 * 30 + 14;
      gfx.drawString(g[7], cx - gfx.textWidth(g[7]) / 2, 0);
      for (int z = 1; z <= 118; z++) drawCell(z, z == _cur);
      _full = false;
    } else {
      if (_prev) drawCell(_prev, false);
      drawCell(_cur, true);
    }
    const Element& e = kElements[_cur - 1];
    gfx.fillRect(0, HDR + ROWS * CH + 1, SCREEN_W, 20, C_BG);
    gfx.setFont(UI_FONT);
    char buf[16];
    int y = HDR + ROWS * CH + 3, x = 8;
    snprintf(buf, sizeof(buf), "%d", e.z);
    gfx.setTextColor(C_ACCENT, C_BG); gfx.drawString(buf, x, y); x += gfx.textWidth(buf) + 10;
    gfx.setTextColor(C_FG, C_BG);     gfx.drawString(e.sym, x, y); x += gfx.textWidth(e.sym) + 10;
    gfx.drawString(e.name, x, y);
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString(e.mass, SCREEN_W - 8 - gfx.textWidth(e.mass), y);
  }

  void field(int y, const char* label, const char* value) {
    gfx.fillRect(0, y, SCREEN_W, 17, C_BG);
    gfx.setFont(UI_FONT);
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString(label, ui::PAD, y);
    gfx.setTextColor(C_FG, C_BG);
    gfx.drawString(value, SCREEN_W - 8 - gfx.textWidth(value), y);
  }

  void drawDetail() {
    if (_full) { gfx.fillScreen(C_BG); _full = false; }
    const Element& e = kElements[_cur - 1];
    uint16_t col = catColor(e.cat);
    gfx.fillRect(0, 0, SCREEN_W, ui::TITLE_H, col);
    gfx.setFont(UI_FONT);
    gfx.setTextColor(RGB565(0, 0, 0));
    char buf[96];
    snprintf(buf, sizeof(buf), "%d   %s — %s", e.z, e.sym, e.name);
    gfx.drawString(buf, 6, 4);

    int y = ui::LIST_TOP;
    const int step = 17;
    snprintf(buf, sizeof(buf), "%d", e.z);                     field(y, "Номер", buf);            y += step;
    field(y, "Атомная масса", e.mass);                                                            y += step;
    snprintf(buf, sizeof(buf), "%d", e.period);                field(y, "Период", buf);           y += step;
    snprintf(buf, sizeof(buf), "%s %s", kRoman[e.group], e.sub == 'A' ? "А" : "Б");
    field(y, "Группа", buf);                                                                      y += step;
    field(y, "Вид", kElemCatNames[e.cat]);                                                       y += step;
    field(y, "Валентность", e.val);                                                              y += step;
    snprintf(buf, sizeof(buf), "%d", e.outer);                 field(y, "Внешних электронов", buf); y += step;

    // описание с прокруткой
    int top = y + 4, bottom = ui::FOOT_Y;
    gfx.fillRect(0, top, SCREEN_W, bottom - top, C_BG);
    gfx.setClipRect(0, top, SCREEN_W, bottom - top);
    rich::setColor(C_FG);
    int h = rich::paragraph(e.desc, ui::PAD, top - _scroll, SCREEN_W - 2 * ui::PAD, true, 16);
    gfx.clearClipRect();
    _maxScroll = h - (bottom - top);
    if (_maxScroll < 0) _maxScroll = 0;
    if (_scroll > _maxScroll) _scroll = _maxScroll;
    ui::footer("Fn+, Fn+/ элемент  Fn+; Fn+. текст");
  }
};
inline App* createPeriodic() { return new PeriodicApp(); }
