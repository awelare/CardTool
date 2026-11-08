#pragma once
#include <stdio.h>
#include <string.h>
#include <esp_system.h>
#include "../app.h"
#include "../ui.h"

class MinesweeperApp : public App {
public:
  void start() override {
    randomSeed(esp_random());
    _screen = DiffSelect;
    _diffSel = 1;
    _cc = 12; _cr = 10; _cm = 20;
    clampCustom();
    _needFullRedraw = true;
  }

  void onKey(const KeyEvent& e) override {
    if (_screen == DiffSelect)  return onKeyDiff(e);
    if (_screen == CustomSetup) return onKeyCustom(e);
    return onKeyGame(e);
  }

  void update(uint32_t nowMs) override {
    if (_screen != Game || _state != Playing || !_firstClickDone) return;
    if ((uint32_t)(nowMs - _lastSec) >= 1000) {
      _lastSec = nowMs;
      _elapsedSec++;
      _needHeader = true;      // только 24px полоса сверху
      invalidate();
    }
  }

  void draw() override {
    gfx.startWrite();

    if (_screen != Game) {
      gfx.fillScreen(C_BG);
      if (_screen == DiffSelect) {
        drawDiffSelect();
      } else {
        drawCustomSetup();
      }
      _needFullRedraw = true;  // при возврате в игру стартуем с чистого листа
      gfx.endWrite();
      return;
    }

    if (_needFullRedraw) {
      gfx.fillScreen(C_BG);
      _needFullRedraw = false;
      _needHeader = true;
      _needFooter = true;
      _needBoardBg = true;
    }

    if (_needHeader) { drawTitleGame(); _needHeader = false; }

    if (_needBoardBg) {
      gfx.fillRect(_ox, _oy, _cols * _cellSize, _rows * _cellSize, 0x0000);
      for (int i = 0; i < _cols * _rows; i++) _dirty[i] = 1;
      _needBoardBg = false;
    }

    int total = _cols * _rows;
    for (int i = 0; i < total; i++) {
      if (_dirty[i]) { drawCell(i % _cols, i / _cols); _dirty[i] = 0; }
    }

    if (_needFooter) { drawFooterGame(); _needFooter = false; }

    gfx.endWrite();
  }

private:
  enum Screen : uint8_t { DiffSelect, CustomSetup, Game };
  enum Diff   : uint8_t { EASY, MEDIUM, HARD, EXPERT, CUSTOM, DIFF_COUNT };
  enum State  : uint8_t { Playing, Won, Lost };

  static constexpr int MAX_CELLS = 20 * 14;
  static constexpr int MIN_COLS = 5,  MAX_COLS = 20;
  static constexpr int MIN_ROWS = 5,  MAX_ROWS = 14;

  Screen _screen = DiffSelect;
  Diff   _diff   = MEDIUM;
  int    _diffSel = 1;
  State  _state  = Playing;

  int _cols = 0, _rows = 0, _mines = 0;
  int _cellSize = 0, _ox = 0, _oy = 0;

  uint8_t _mine[MAX_CELLS];
  uint8_t _open[MAX_CELLS];
  uint8_t _flag[MAX_CELLS];
  uint8_t _adj [MAX_CELLS];

  uint8_t _dirty[MAX_CELLS];      // 1 = клетку надо перерисовать
  bool _needFullRedraw = true;
  bool _needHeader     = false;
  bool _needFooter     = false;
  bool _needBoardBg    = false;

  int  _curC = 0, _curR = 0;
  bool _firstClickDone = false;

  uint32_t _elapsedSec = 0;
  uint32_t _lastSec    = 0;

  int _cc = 12, _cr = 10, _cm = 20;
  int _customSel = 0;

  static const char* diffName(Diff d) {
    static const char* n[] = {"Простой", "Средний", "Трудный", "Очень трудный", "Свой"};
    return n[(int)d];
  }
  static void diffPreset(Diff d, int& c, int& r, int& m) {
    switch (d) {
      case EASY:   c =  9; r =  9; m = 10; break;
      case MEDIUM: c = 12; r = 10; m = 20; break;
      case HARD:   c = 16; r = 12; m = 38; break;
      case EXPERT: c = 20; r = 14; m = 60; break;
      default:     c = 12; r = 10; m = 20; break;
    }
  }
  static int maxMinesFor(int c, int r) {
    int cells = c * r;
    int safeReserve = cells - 9;
    int densityCap  = (cells * 30) / 100;
    int lim = safeReserve < densityCap ? safeReserve : densityCap;
    return lim < 1 ? 1 : lim;
  }
  void clampCustom() {
    if (_cc < MIN_COLS) _cc = MIN_COLS;
    if (_cc > MAX_COLS) _cc = MAX_COLS;
    if (_cr < MIN_ROWS) _cr = MIN_ROWS;
    if (_cr > MAX_ROWS) _cr = MAX_ROWS;
    int m = maxMinesFor(_cc, _cr);
    if (_cm < 1) _cm = 1;
    if (_cm > m) _cm = m;
  }

  // ==== ввод ====
  void onKeyDiff(const KeyEvent& e) {
    if (e.key == Key::ESC) { requestExit(); return; }
    if (e.key == Key::UP   || (e.key == Key::CHAR && e.ch == ';')) { if (_diffSel > 0) { _diffSel--; invalidate(); } return; }
    if (e.key == Key::DOWN || (e.key == Key::CHAR && e.ch == '.')) { if (_diffSel < DIFF_COUNT-1) { _diffSel++; invalidate(); } return; }
    if (e.key == Key::ENTER) {
      _diff = (Diff)_diffSel;
      if (_diff == CUSTOM) { _screen = CustomSetup; invalidate(); return; }
      _screen = Game; newGame();
    }
  }

  void onKeyCustom(const KeyEvent& e) {
    if (e.key == Key::ESC)   { _screen = DiffSelect; invalidate(); return; }
    if (e.key == Key::ENTER) { _screen = Game; newGame(); return; }
    int dir = 0, val = 0;
    switch (e.key) {
      case Key::UP:    dir = -1; break;
      case Key::DOWN:  dir =  1; break;
      case Key::LEFT:  val = -1; break;
      case Key::RIGHT: val =  1; break;
      case Key::CHAR:
        if (e.ch == ';') dir = -1;
        else if (e.ch == '.') dir = 1;
        else if (e.ch == ',') val = -1;
        else if (e.ch == '/') val = 1;
        else return;
        break;
      default: return;
    }
    if (dir) {
      _customSel += dir;
      if (_customSel < 0) _customSel = 0;
      if (_customSel > 2) _customSel = 2;
      invalidate();
    }
    if (val) {
      if (_customSel == 0) _cc += val;
      else if (_customSel == 1) _cr += val;
      else _cm += val;
      clampCustom();
      invalidate();
    }
  }

  void onKeyGame(const KeyEvent& e) {
    if (e.key == Key::ESC) { _screen = DiffSelect; invalidate(); return; }

    if (_state == Won || _state == Lost) {
      if (e.key == Key::ENTER) newGame();
      return;
    }

    int ncol = _curC, nrow = _curR;
    switch (e.key) {
      case Key::UP:    if (nrow > 0) nrow--; break;
      case Key::DOWN:  if (nrow < _rows - 1) nrow++; break;
      case Key::LEFT:  if (ncol > 0) ncol--; break;
      case Key::RIGHT: if (ncol < _cols - 1) ncol++; break;
      case Key::DEL: {
        int i = nrow * _cols + ncol;
        if (!_open[i]) {
          _flag[i] = !_flag[i];
          _dirty[i] = 1;
          _needHeader = true;        // счётчик флагов изменился
          invalidate();
        }
        return;
      }
      case Key::ENTER:
        _curC = ncol; _curR = nrow;
        revealAt(ncol, nrow);
        return;
      case Key::CHAR:
        if (e.ch == ';') { if (nrow > 0) nrow--; }
        else if (e.ch == '.') { if (nrow < _rows - 1) nrow++; }
        else if (e.ch == ',') { if (ncol > 0) ncol--; }
        else if (e.ch == '/') { if (ncol < _cols - 1) ncol++; }
        else if (e.ch == ' ') { _curC = ncol; _curR = nrow; revealAt(ncol, nrow); return; }
        else return;
        break;
      default: return;
    }
    if (ncol != _curC || nrow != _curR) {
      _dirty[_curR * _cols + _curC] = 1;   // старый курсор
      _curC = ncol; _curR = nrow;
      _dirty[_curR * _cols + _curC] = 1;   // новый курсор
      invalidate();
    }
  }

  // ==== игровая логика ====
  void newGame() {
    if (_diff == CUSTOM) { _cols = _cc; _rows = _cr; _mines = _cm; }
    else                 { diffPreset(_diff, _cols, _rows, _mines); }

    int total = _cols * _rows;
    memset(_mine, 0, total);
    memset(_open, 0, total);
    memset(_flag, 0, total);
    memset(_adj,  0, total);
    memset(_dirty, 0, total);

    int availW = 316, availH = 186;
    int cs = availW / _cols;
    if (availH / _rows < cs) cs = availH / _rows;
    if (cs > 30) cs = 30;
    if (cs < 10) cs = 10;
    _cellSize = cs;

    int bw = _cols * _cellSize, bh = _rows * _cellSize;
    _ox = (320 - bw) / 2;
    _oy = 28 + (190 - bh) / 2;

    _curC = _cols / 2;
    _curR = _rows / 2;
    _firstClickDone = false;
    _state = Playing;
    _elapsedSec = 0;
    _lastSec = millis();
    _needFullRedraw = true;
    invalidate();
  }

  void placeMines(int safeIdx) {
    int sr = safeIdx / _cols, sc = safeIdx % _cols;
    bool safeZone = (_diff == EASY || _diff == MEDIUM || _diff == CUSTOM);
    int placed = 0, guard = 0;
    while (placed < _mines && guard < 20000) {
      guard++;
      int r = (int)random(_rows);
      int c = (int)random(_cols);
      int i = r * _cols + c;
      if (_mine[i]) continue;
      if (i == safeIdx) continue;
      if (safeZone) {
        int dr = r - sr; if (dr < 0) dr = -dr;
        int dc = c - sc; if (dc < 0) dc = -dc;
        if (dr <= 1 && dc <= 1) continue;
      }
      _mine[i] = 1;
      placed++;
    }
    for (int i = 0; i < _cols * _rows; i++) {
      if (_mine[i]) continue;
      int r = i / _cols, c = i % _cols;
      int cnt = 0;
      for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
          if (!dx && !dy) continue;
          int nr = r + dy, nc = c + dx;
          if (nr < 0 || nc < 0 || nr >= _rows || nc >= _cols) continue;
          if (_mine[nr * _cols + nc]) cnt++;
        }
      _adj[i] = cnt;
    }
  }

  void floodOpen(int start) {
    int st[MAX_CELLS]; int top = 0;
    _open[start] = 1;
    _dirty[start] = 1;
    st[top++] = start;
    while (top > 0) {
      int idx = st[--top];
      if (_adj[idx] != 0) continue;
      int rr = idx / _cols, cc = idx % _cols;
      for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
          if (!dx && !dy) continue;
          int nr = rr + dy, nc = cc + dx;
          if (nr < 0 || nc < 0 || nr >= _rows || nc >= _cols) continue;
          int ni = nr * _cols + nc;
          if (_open[ni] || _flag[ni] || _mine[ni]) continue;
          _open[ni] = 1;
          _dirty[ni] = 1;
          st[top++] = ni;
        }
    }
  }

  void revealAt(int c, int r) {
    if (_state != Playing) return;
    int i = r * _cols + c;
    if (_flag[i] || _open[i]) return;

    if (!_firstClickDone) {
      placeMines(i);
      _firstClickDone = true;
      _lastSec = millis();
    }
    if (_mine[i]) {
      _open[i] = 1;
      _state = Lost;
      int total = _cols * _rows;
      for (int j = 0; j < total; j++) _dirty[j] = 1;
      _needFooter = true;
      invalidate();
      return;
    }
    floodOpen(i);
    checkWin();
    invalidate();
  }

  void checkWin() {
    int total = _cols * _rows;
    for (int i = 0; i < total; i++)
      if (!_mine[i] && !_open[i]) return;
    _state = Won;
    for (int i = 0; i < total; i++) {
      if (_mine[i]) _flag[i] = 1;
      _dirty[i] = 1;
    }
    _needHeader = true;
    _needFooter = true;
  }

  int countFlags() const {
    int n = 0, total = _cols * _rows;
    for (int i = 0; i < total; i++) if (_flag[i]) n++;
    return n;
  }

  static uint16_t numColor(int n) {
    switch (n) {
      case 1: return 0x001F; case 2: return 0x0400;
      case 3: return 0xC000; case 4: return 0x4010;
      case 5: return 0x8200; case 6: return 0x0410;
      case 7: return 0x0000; case 8: return 0x4208;
      default: return 0x0000;
    }
  }

  // ==== отрисовка ====
  void drawDiffSelect() {
    ui::titleBar("Сапёр — сложность");
    for (int i = 0; i < DIFF_COUNT; i++) {
      int y = ui::LIST_TOP + i * ui::ROW_H;
      char val[24];
      if (i == CUSTOM) snprintf(val, sizeof(val), "%dx%d", _cc, _cr);
      else { int c, r, m; diffPreset((Diff)i, c, r, m); snprintf(val, sizeof(val), "%dx%d  %dм", c, r, m); }
      ui::row(y, diffName((Diff)i), val, i == _diffSel);
    }
    ui::footer("; . — выбор   Enter — играть   Esc");
  }

  void drawCustomSetup() {
    ui::titleBar("Сапёр — своё поле");
    int maxM = maxMinesFor(_cc, _cr);
    char vbuf[24];
    snprintf(vbuf, sizeof(vbuf), "%d (5-20)", _cc);
    ui::row(ui::LIST_TOP + 0*ui::ROW_H, "Ширина", vbuf, _customSel == 0);
    snprintf(vbuf, sizeof(vbuf), "%d (5-14)", _cr);
    ui::row(ui::LIST_TOP + 1*ui::ROW_H, "Высота", vbuf, _customSel == 1);
    snprintf(vbuf, sizeof(vbuf), "%d (1-%d)", _cm, maxM);
    ui::row(ui::LIST_TOP + 2*ui::ROW_H, "Мины",   vbuf, _customSel == 2);
    char info[48];
    int cells = _cc * _cr;
    int dens = (_cm * 100) / cells;
    snprintf(info, sizeof(info), "Клеток: %d   Плотность: %d%%", cells, dens);
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString(info, ui::PAD, ui::LIST_TOP + 3*ui::ROW_H + 8);
    ui::footer("; . , / изм.  Enter старт  Esc");
  }

  void drawTitleGame() {
    char t[56];
    int mins = (int)(_elapsedSec / 60), secs = (int)(_elapsedSec % 60);
    snprintf(t, sizeof(t), "Сапёр %s  %d/%d  %d:%02d",
             diffName(_diff), _mines - countFlags(), _mines, mins, secs);
    ui::titleBar(t);
  }

  void drawFooterGame() {
    if (_state == Won)       ui::footer("Победа!  Enter — новое   Esc — назад");
    else if (_state == Lost) ui::footer("Взрыв!  Enter — новое   Esc — назад");
    else                     ui::footer("Enter — открыть   Del — флаг   Esc — выход");
  }

  void drawCell(int c, int r) {
    int x = _ox + c * _cellSize;
    int y = _oy + r * _cellSize;
    int i = r * _cols + c;
    int sz = _cellSize - 1; if (sz < 1) sz = 1;

    uint16_t bg;
    if (_open[i]) {
      if (_mine[i] && _state == Lost) bg = 0xF800;
      else                            bg = 0xC618;
    } else {
      if (_mine[i] && _state == Lost) bg = 0xF800;
      else                            bg = 0x6B4D;
    }
    gfx.fillRect(x, y, sz, sz, bg);

    if (_open[i] && !_mine[i] && _adj[i] > 0) {
      char buf[2] = { (char)('0' + _adj[i]), 0 };
      int tw = gfx.textWidth(buf), th = gfx.fontHeight();
      gfx.setTextColor(numColor(_adj[i]), bg);
      gfx.drawString(buf, x + (sz - tw) / 2, y + (sz - th) / 2);
    } else if (_mine[i] && (_open[i] || _state == Lost)) {
      gfx.fillCircle(x + sz / 2, y + sz / 2, sz / 4 > 1 ? sz / 4 : 2, 0x0000);
    } else if (!_open[i] && _flag[i]) {
      gfx.fillRect(x + sz / 2, y + sz / 4, 1, sz / 2, 0x0000);
      gfx.fillTriangle(x + sz / 2 + 1, y + sz / 4,
                       x + sz / 2 + sz / 3, y + sz / 4 + 2,
                       x + sz / 2 + 1, y + sz / 2, 0xF800);
    }

    if (c == _curC && r == _curR && _state == Playing)
      gfx.drawRect(x, y, sz, sz, C_ACCENT);
  }
};

inline App* createMinesweeper() { return new MinesweeperApp(); }