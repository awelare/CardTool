#pragma once
#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "../app.h"
#include "../ui.h"
#include "../display.h"

class PeriodicApp : public App {
public:
  void start() override {
    _scr = 0; _col = 0; _row = 0; _ln = 0; _det = 0; _dy = 0;
    _selZ = 0; _cellP = 0; _cellG = 0; _hasMark = 0; _err = false;
    memset(_els, 0, sizeof(_els));
    if (!g_sdOk) { _err = true; invalidate(); return; }
    File f = SD.open("/school/periodic.json");
    if (!f) { _err = true; invalidate(); return; }
    if (deserializeJson(_doc, f)) { _err = true; f.close(); invalidate(); return; }
    f.close();
    for (JsonObject o : _doc.as<JsonArray>()) {
      int z = o["z"] | 0;
      if (z < 1 || z > 118) continue;
      Elem& e = _els[z]; e.z = z;
      strncpy(e.sym,  o["sym"]  | "", 3);  e.sym[3]  = 0;
      strncpy(e.name, o["name"] | "", 23); e.name[23] = 0;
      e.mass = o["mass"] | 0.0f;
      strncpy(e.val,  o["val"]  | "", 15); e.val[15] = 0;
      e.type = o["type"] | 6;
      e.per  = o["per"]  | 0;
      strncpy(e.grp,  o["group"] | "", 7); e.grp[7] = 0;
      e.elec = o["elec"] | 0;
      strncpy(e.desc, o["desc"]  | "", 79); e.desc[79] = 0;
    }
    invalidate();
  }

  void onKey(const KeyEvent& e) override {
    if (_err) { if (e.key == Key::ESC) requestExit(); return; }
    if (_det) {
      if (e.key == Key::ESC) { _det = 0; invalidate(); return; }
      if (e.key == Key::UP || e.ch == ';')   { if (_dy > 0) { _dy--; invalidate(); } return; }
      if (e.key == Key::DOWN || e.ch == '.') { _dy++; invalidate(); return; }
      if (e.key == Key::ENTER && _hasMark) { _det = 0; _scr = _hasMark; _ln = 0; invalidate(); return; }
      return;
    }
    if (e.key == Key::ESC) {
      if (_scr) { _scr = 0; invalidate(); return; }
      requestExit(); return;
    }
    if (_scr == 0) {
      if (e.key == Key::LEFT  || e.ch == ',') { if (_col > 0) { _col--; invalidate(); } return; }
      if (e.key == Key::RIGHT || e.ch == '/') { if (_col < 7) { _col++; invalidate(); } return; }
      if (e.key == Key::UP    || e.ch == ';') { if (_row > 0) { _row--; invalidate(); } return; }
      if (e.key == Key::DOWN  || e.ch == '.') { if (_row < 6) { _row++; invalidate(); } return; }
      if (e.key == Key::ENTER) {
        int p = _row + 1, g = _col + 1;
        fillCell(p, g);
        _hasMark = 0;
        bool hasReal = false;
        for (int i = 0; i < _nz; i++) {
          if (_zs[i] == -1) _hasMark = 1;
          else if (_zs[i] == -2) _hasMark = 2;
          else if (_zs[i] > 0) hasReal = true;
        }
        if (hasReal || _hasMark) { _det = 1; _dy = 0; _cellP = p; _cellG = g; invalidate(); }
        return;
      }
    } else {
      if (e.key == Key::LEFT  || e.ch == ',') { if (_ln > 0)  { _ln--; invalidate(); } return; }
      if (e.key == Key::RIGHT || e.ch == '/') { if (_ln < 13) { _ln++; invalidate(); } return; }
      if (e.key == Key::ENTER) {
        int base = (_scr == 1) ? 58 : 90;
        _det = 1; _dy = 0; _cellP = -1; _cellG = -1;
        _selZ = base + _ln; _hasMark = 0;
        invalidate(); return;
      }
    }
  }

  void draw() override {
    gfx.startWrite(); gfx.fillScreen(C_BG);
    if (_err) {
      ui::titleBar("Таблица Менделеева");
      gfx.setTextColor(C_ERR, C_BG);
      gfx.drawString(!g_sdOk ? "SD не найдена" : "Нет /school/periodic.json", ui::PAD, ui::LIST_TOP + 10);
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString("Проверь имя, BOM и путь", ui::PAD, ui::LIST_TOP + 30);
      ui::footer("Esc выход"); gfx.endWrite(); return;
    }
    if (_det) drawDetail();
    else if (_scr == 0) drawMain();
    else drawLnAn();
    gfx.endWrite();
  }

private:
  struct Elem {
    uint8_t z, type, per, elec;
    char sym[4], name[24], val[16], grp[8], desc[80];
    float mass;
  };
  Elem _els[119];
  JsonDocument _doc;
  int _scr, _col, _row, _ln, _det, _dy, _selZ, _cellP, _cellG, _hasMark;
  bool _err;
  int _zs[4], _nz;

  uint16_t col(uint8_t t) {
    switch (t) {
      case 1:  return RGB565(190,  60, 110);
      case 2:  return RGB565(245, 150,  60);
      case 3:  return RGB565( 70, 130, 220);
      case 4:  return RGB565(180, 180, 190);
      case 5:  return RGB565(160, 210, 160);
      case 6:  return RGB565(255, 220,  90);
      case 7:  return RGB565(255, 235, 110);
      case 8:  return RGB565(180, 120, 210);
      case 9:  return RGB565( 80, 200, 100);
      case 10: return RGB565( 50, 150,  70);
    }
    return C_DIM;
  }

  void fillCell(int p, int g) {
    _nz = 0;
    if (p == 1) { if (g == 1) _zs[_nz++] = 1; else if (g == 8) _zs[_nz++] = 2; return; }
    if (p == 2) { _zs[_nz++] = 3 + g - 1; return; }
    if (p == 3) { _zs[_nz++] = 11 + g - 1; return; }
    static const uint8_t perA[4][8] = {
      {19, 20, 31, 32, 33, 34, 35, 36},
      {37, 38, 49, 50, 51, 52, 53, 54},
      {55, 56, 81, 82, 83, 84, 85, 86},
      {87, 88, 113, 114, 115, 116, 117, 118}
    };
    static const uint8_t perB[4][8] = {
      {29, 30, 21, 22, 23, 24, 25, 26},
      {47, 48, 39, 40, 41, 42, 43, 44},
      {79, 80, 57, 72, 73, 74, 75, 76},
      {111, 112, 89, 104, 105, 106, 107, 108}
    };
    _zs[_nz++] = perA[p - 4][g - 1];
    if (g == 3 && p >= 6) { _zs[_nz++] = (p == 6) ? -1 : -2; return; }
    if (g == 8) {
      _zs[_nz++] = perB[p - 4][g - 1];
      _zs[_nz++] = perB[p - 4][g - 1] + 1;
      _zs[_nz++] = perB[p - 4][g - 1] + 2;
    } else {
      _zs[_nz++] = perB[p - 4][g - 1];
    }
  }

  // Точное центрирование по fontHeight
  void drawOne(int x, int y, int w, int h, int z, uint16_t bg = 0) {
    if (z < 1 || z > 118) return;
    uint16_t c = bg ? bg : col(_els[z].type);
    gfx.fillRect(x, y, w - 1, h - 1, c);
    gfx.setTextColor(TFT_BLACK, c); gfx.setTextSize(1);
    int fh = gfx.fontHeight();
    int fw = gfx.textWidth(_els[z].sym);
    int tx = x + (w - 1 - fw) / 2;
    int ty = y + (h - 1 - fh) / 2;
    if (ty < y) ty = y;
    if (tx < x) tx = x;
    gfx.drawString(_els[z].sym, tx, ty);
  }

  void drawCell(int x, int y, int w, int h) {
    int hUp = h / 2;
    int hDn = h - hUp;
    int zA = _zs[0];
    gfx.fillRect(x, y, w - 1, hUp, col(_els[zA].type));
    int zB = _zs[1];
    uint16_t bgB = (zB == -1) ? col(9) : (zB == -2) ? col(10) : col(_els[zB].type);
    gfx.fillRect(x, y + hUp, w - 1, hDn - 1, bgB);
    gfx.setTextColor(TFT_BLACK); gfx.setTextSize(1);
    int fh = gfx.fontHeight();
    // A — верхняя половина
    int fw = gfx.textWidth(_els[zA].sym);
    int ty = y + (hUp - fh) / 2;
    if (ty < y) ty = y;
    gfx.drawString(_els[zA].sym, x + (w - 1 - fw) / 2, ty);
    // B — нижняя половина
    char trio[14];
    const char* txt;
    if (zB < 0) txt = (zB == -1) ? "La-Lu" : "Ac-Lr";
    else if (_nz == 4) {
      snprintf(trio, sizeof(trio), "%s%s%s",
        _els[_zs[1]].sym, _els[_zs[2]].sym, _els[_zs[3]].sym);
      txt = trio;
      if (gfx.textWidth(txt) > w - 2) txt = _els[_zs[1]].sym;
    } else txt = _els[zB].sym;
    fw = gfx.textWidth(txt);
    ty = y + hUp + (hDn - fh) / 2;
    if (ty < y + hUp) ty = y + hUp;
    gfx.drawString(txt, x + (w - 1 - fw) / 2, ty);
  }

  void drawMain() {
    ui::titleBar("Таблица Менделеева");
    const int LX = 12, CW = 38, HY = 24, HH = 14;
    const char* gn[] = {"I","II","III","IV","V","VI","VII","VIII"};
    gfx.setTextColor(C_DIM, C_BG); gfx.setTextSize(1);
    int fh = gfx.fontHeight();
    for (int g = 0; g < 8; g++) {
      int tw = gfx.textWidth(gn[g]);
      gfx.drawString(gn[g], LX + g * CW + (CW - tw) / 2, HY + (HH - fh) / 2);
    }
    int yy = HY + HH;
    for (int p = 1; p <= 3; p++) {
      char pb[4]; snprintf(pb, sizeof(pb), "%d", p);
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString(pb, 2, yy + (16 - fh) / 2);
      for (int g = 1; g <= 8; g++) {
        int cx = LX + (g - 1) * CW;
        fillCell(p, g);
        if (_nz) drawOne(cx, yy, CW, 16, _zs[0]);
      }
      yy += 16;
    }
    for (int p = 4; p <= 7; p++) {
      char pb[4]; snprintf(pb, sizeof(pb), "%d", p);
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString(pb, 2, yy + (22 - fh) / 2);
      for (int g = 1; g <= 8; g++) {
        int cx = LX + (g - 1) * CW;
        fillCell(p, g);
        if (_nz) drawCell(cx, yy, CW, 22);
      }
      yy += 22;
    }
    int LNW = 308 / 14;
    for (int i = 0; i < 14; i++) {
      int z = 58 + i;
      if (_els[z].z == z) drawOne(LX + i * LNW, yy, LNW, 16, z, col(9));
    }
    yy += 16;
    for (int i = 0; i < 14; i++) {
      int z = 90 + i;
      if (_els[z].z == z) drawOne(LX + i * LNW, yy, LNW, 16, z, col(10));
    }
    int cx, cy, cw, ch;
    if (_row < 3) { cx = LX + _col * CW; cy = HY + HH + _row * 16; cw = CW; ch = 16; }
    else          { cx = LX + _col * CW; cy = HY + HH + 48 + (_row - 3) * 22; cw = CW; ch = 22; }
    gfx.drawRect(cx, cy, cw - 1, ch - 1, C_ACCENT);
    gfx.drawRect(cx + 1, cy + 1, cw - 3, ch - 3, C_ACCENT);
    ui::footer("Стрелки Enter Esc");
  }

  void drawLnAn() {
    ui::titleBar(_scr == 1 ? "Лантаноиды" : "Актиноиды");
    int base = (_scr == 1) ? 58 : 90;
    int LNW = 320 / 14;
    int fh = gfx.fontHeight();
    for (int i = 0; i < 14; i++) {
      int z = base + i;
      if (_els[z].z != z) continue;
      int x = i * LNW, y = 40;
      gfx.fillRect(x, y, LNW - 1, 100, col(_els[z].type));
      gfx.setTextColor(TFT_BLACK); gfx.setTextSize(1);
      int tw = gfx.textWidth(_els[z].sym);
      gfx.drawString(_els[z].sym, x + (LNW - tw) / 2, y + (100 - 2*fh - 4) / 2);
      char zb[6]; snprintf(zb, sizeof(zb), "%d", z);
      tw = gfx.textWidth(zb);
      gfx.drawString(zb, x + (LNW - tw) / 2, y + (100 - 2*fh - 4) / 2 + fh + 4);
      if (i == _ln) gfx.drawRect(x, y, LNW - 1, 100, C_ACCENT);
    }
    ui::footer("Стрелки Enter Esc назад");
  }

  // Перенос по словам — возвращает y после последней строки
  int wrapText(const char* text, int x, int y0, int maxW, int maxY, uint16_t color) {
    gfx.setTextColor(color, C_BG);
    gfx.setTextSize(1);
    int fh = gfx.fontHeight();
    if (fh < 10) fh = 12;
    int y = y0;
    const char* p = text;
    while (*p && y + fh <= maxY) {
      int len = 0, lastSpace = -1;
      char buf[100];
      while (p[len] && len < 99) {
        buf[len] = p[len];
        buf[len + 1] = 0;
        if (gfx.textWidth(buf) > maxW) break;
        if (p[len] == ' ') lastSpace = len;
        len++;
      }
      if (len == 0) break;
      int cut = len;
      if (p[len] != 0 && lastSpace > 0) cut = lastSpace;
      char line[100];
      strncpy(line, p, cut);
      line[cut] = 0;
      gfx.drawString(line, x, y);
      y += fh;
      p += cut;
      while (*p == ' ') p++;
    }
    return y;
  }

  void drawDetail() {
    ui::titleBar(_cellP > 0 ? "Элемент" : (_scr == 1 ? "Лантаноид" : "Актиноид"));
    int maxY = ui::FOOT_Y - 4;
    int y = ui::LIST_TOP - _dy * 14;
    if (_cellP > 0) {
      fillCell(_cellP, _cellG);
      for (int i = 0; i < _nz; i++) {
        int z = _zs[i];
        if (z < 0) continue;
        y = drawElemInfo(z, y, maxY);
      }
      if (_hasMark && y + 30 < maxY) {
        gfx.setTextColor(C_OK, C_BG);
        gfx.drawString(_hasMark == 1 ? "Лантаноиды La-Lu" : "Актиноиды Ac-Lr", ui::PAD, y);
        y += 14;
        gfx.setTextColor(C_DIM, C_BG);
        gfx.drawString("Enter — открыть список", ui::PAD, y);
      }
    } else {
      y = drawElemInfo(_selZ, y, maxY);
    }
    if (_hasMark) ui::footer("Enter лантаноиды  Esc назад");
    else          ui::footer("Вверх/вниз скролл  Esc назад");
  }

  int drawElemInfo(int z, int y, int maxY) {
    if (z < 1 || z > 118) return y;
    Elem& e = _els[z];
    char b[80];
    int fh = gfx.fontHeight();
    if (fh < 10) fh = 12;
    int lineH = fh + 2;
    gfx.setTextSize(1);
    if (y + lineH < maxY) {
      gfx.setTextColor(C_ACCENT, C_BG);
      snprintf(b, sizeof(b), "%s (%d) %s", e.sym, e.z, e.name);
      gfx.drawString(b, ui::PAD, y);
    }
    y += lineH;
    gfx.setTextColor(C_FG, C_BG);
    if (y + lineH < maxY) { snprintf(b, sizeof(b), "Масса: %.2f  Вал: %s", e.mass, e.val); gfx.drawString(b, ui::PAD, y); } y += lineH;
    if (y + lineH < maxY) { snprintf(b, sizeof(b), "Период: %d  Группа: %s", e.per, e.grp); gfx.drawString(b, ui::PAD, y); } y += lineH;
    if (y + lineH < maxY) { snprintf(b, sizeof(b), "Электронов: %d", e.elec); gfx.drawString(b, ui::PAD, y); } y += lineH;
    if (y + lineH < maxY) {
      const char* tn[] = {"", "Щелочной", "Щёлочноземельный", "Переходный", "Постпереходный", "Полуметалл", "Неметалл", "Галоген", "Благородный газ", "Лантаноид", "Актиноид"};
      snprintf(b, sizeof(b), "Вид: %s", (e.type >= 1 && e.type <= 10) ? tn[e.type] : "?");
      gfx.drawString(b, ui::PAD, y);
    }
    y += lineH;
    if (y + lineH < maxY && e.desc[0]) {
      y = wrapText(e.desc, ui::PAD, y, SCREEN_W - 2 * ui::PAD, maxY, C_DIM);
    }
    y += 8;
    return y;
  }
};
inline App* createPeriodic() { return new PeriodicApp(); }