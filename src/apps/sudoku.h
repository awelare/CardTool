#pragma once
#include <stdio.h>
#include <string.h>
#include <esp_system.h>
#include "../app.h"
#include "../ui.h"

class SudokuApp : public App {
public:
  enum Screen { DiffSelect, Game };
  enum Diff { EASY, MEDIUM, HARD, EXPERT, DIFF_COUNT };

  void start() override {
    randomSeed(esp_random());
    _screen = DiffSelect;
    _diffSel = 1;
  }

  void onKey(const KeyEvent& e) override {
    if (_screen == DiffSelect) {
      if (e.key == Key::ESC) { requestExit(); return; }
      if (e.key == Key::UP || (e.key == Key::CHAR && e.ch == ';')) {
        if (_diffSel > 0) { _diffSel--; invalidate(); }
        return;
      }
      if (e.key == Key::DOWN || (e.key == Key::CHAR && e.ch == '.')) {
        if (_diffSel < DIFF_COUNT - 1) { _diffSel++; invalidate(); }
        return;
      }
      if (e.key == Key::ENTER) {
        _diff = (Diff)_diffSel;
        _screen = Game;
        _won = false;
        showGenerating();
        newPuzzle();
        return;
      }
      return;
    }

    if (e.key == Key::ESC) { _screen = DiffSelect; invalidate(); return; }

    if (_won) {
      if (e.key == Key::ENTER) { showGenerating(); newPuzzle(); }
      return;
    }

    int oldR = _row, oldC = _col;
    switch (e.key) {
      case Key::UP:    if (_row > 0) _row--; break;
      case Key::DOWN:  if (_row < 8) _row++; break;
      case Key::LEFT:  if (_col > 0) _col--; break;
      case Key::RIGHT: if (_col < 8) _col++; break;
      case Key::DEL: {
        int i = _row * 9 + _col;
        if (!_given[i]) { _grid[i] = 0; _badCell = -1; }
        break;
      }
      case Key::CHAR:
        if (e.ch == ';') { if (_row > 0) _row--; }
        else if (e.ch == '.') { if (_row < 8) _row++; }
        else if (e.ch == ',') { if (_col > 0) _col--; }
        else if (e.ch == '/') { if (_col < 8) _col++; }
        else if (e.ch >= '1' && e.ch <= '9') {
          int i = _row * 9 + _col;
          if (!_given[i]) {
            int v = e.ch - '0';
            _grid[i] = v;
            _badCell = isValidPlacement(_grid, i, v, true) ? -1 : i;
            checkWin();
          }
        }
        break;
      default: return;
    }
    if (oldR != _row || oldC != _col) _badCell = -1;
    invalidate();
  }

  void draw() override {
    gfx.startWrite();
    gfx.fillScreen(C_BG);
    if (_screen == DiffSelect) drawDiffSelect();
    else                       drawGame();
    gfx.endWrite();
  }

private:
  Screen _screen = DiffSelect;
  Diff _diff = MEDIUM;
  int _diffSel = 1;
  uint8_t _given[81];
  uint8_t _grid[81];
  int _row = 0, _col = 0;
  int _badCell = -1;
  bool _won = false;

  static constexpr int CELL = 20;
  static constexpr int GX = 70;
  static constexpr int GY = 28;
  static constexpr int GW = 9 * CELL;
  static constexpr int GH = 9 * CELL;

  static const char* diffName(Diff d) {
    switch (d) {
      case EASY:   return "Простой";
      case MEDIUM: return "Средний";
      case HARD:   return "Трудный";
      case EXPERT: return "Очень трудный";
      default:     return "?";
    }
  }
  static int targetGivens(Diff d) {
    switch (d) {
      case EASY:   return 40;
      case MEDIUM: return 32;
      case HARD:   return 27;
      case EXPERT: return 23;
      default:     return 32;
    }
  }

  void drawDiffSelect() {
    ui::titleBar("Судоку — сложность");
    for (int i = 0; i < DIFF_COUNT; i++) {
      int y = ui::LIST_TOP + i * ui::ROW_H;
      char val[24];
      snprintf(val, sizeof(val), "~%d цифр", targetGivens((Diff)i));
      ui::row(y, diffName((Diff)i), val, i == _diffSel);
    }
    ui::footer("; . — выбор   Enter — играть   Esc — выход");
  }

  void drawGame() {
    char t[48];
    snprintf(t, sizeof(t), "Судоку — %s", diffName(_diff));
    ui::titleBar(t);
    drawGrid();
    if (_won) ui::footer("Решено!  Enter — новое   Esc — назад");
    else      ui::footer("; . , / — курсор  1-9 — цифра  Esc — назад");
  }

  void drawGrid() {
    for (int r = 0; r < 9; r++)
      for (int c = 0; c < 9; c++) {
        int i = r * 9 + c;
        int x = GX + c * CELL;
        int y = GY + r * CELL;
        bool isCursor = (r == _row && c == _col);
        uint16_t bg = isCursor ? C_SEL : C_BG;
        gfx.fillRect(x + 1, y + 1, CELL - 2, CELL - 2, bg);
        uint8_t v = _grid[i];
        if (v) {
          char buf[2] = { char('0' + v), 0 };
          int tw = gfx.textWidth(buf);
          int th = gfx.fontHeight();
          uint16_t fg;
          if (i == _badCell)   fg = C_ERR;
          else if (_given[i])  fg = C_FG;
          else                 fg = C_ACCENT;
          gfx.setTextColor(fg, bg);
          gfx.drawString(buf, x + (CELL - tw) / 2, y + (CELL - th) / 2);
        }
      }
    for (int i = 0; i <= 9; i++) {
      bool thick = (i % 3 == 0);
      uint16_t col = thick ? C_FG : C_DIM;
      gfx.drawFastVLine(GX + i * CELL, GY, GH, col);
      gfx.drawFastHLine(GX, GY + i * CELL, GW, col);
    }
  }

  void showGenerating() {
    gfx.startWrite();
    gfx.fillScreen(C_BG);
    ui::titleBar("Судоку");
    gfx.setTextColor(C_FG, C_BG);
    gfx.drawString("Генерация...", ui::PAD, ui::LIST_TOP + 8);
    ui::footer("Подождите...");
    gfx.endWrite();
  }

  static bool isValidPlacement(const uint8_t* g, int idx, int v, bool skipSelf = false) {
    int r = idx / 9, c = idx % 9;
    int br = (r / 3) * 3, bc = (c / 3) * 3;
    for (int i = 0; i < 9; i++) {
      int ir = r * 9 + i;
      int ic = i * 9 + c;
      int rr = br + i / 3, cc = bc + i % 3;
      int ib = rr * 9 + cc;
      if (ir != idx || !skipSelf) if (g[ir] == v) return false;
      if (ic != idx || !skipSelf) if (g[ic] == v) return false;
      if (ib != idx || !skipSelf) if (g[ib] == v) return false;
    }
    return true;
  }

  bool fillRandom(uint8_t* g, int idx) {
    if (idx == 81) return true;
    if (g[idx]) return fillRandom(g, idx + 1);
    uint8_t vals[9] = {1,2,3,4,5,6,7,8,9};
    for (int i = 8; i > 0; i--) {
      int j = random(i + 1);
      uint8_t t = vals[i]; vals[i] = vals[j]; vals[j] = t;
    }
    for (int i = 0; i < 9; i++) {
      if (isValidPlacement(g, idx, vals[i])) {
        g[idx] = vals[i];
        if (fillRandom(g, idx + 1)) return true;
        g[idx] = 0;
      }
    }
    return false;
  }

  int countSolutions(uint8_t* g, int maxCount) {
    int bestIdx = -1, bestCnt = 10;
    for (int i = 0; i < 81; i++) {
      if (g[i]) continue;
      int cnt = 0;
      for (int v = 1; v <= 9; v++) if (isValidPlacement(g, i, v)) cnt++;
      if (cnt < bestCnt) {
        bestCnt = cnt; bestIdx = i;
        if (cnt <= 1) break;
      }
    }
    if (bestIdx < 0) return 1;
    if (bestCnt == 0) return 0;
    int count = 0;
    for (int v = 1; v <= 9 && count < maxCount; v++) {
      if (isValidPlacement(g, bestIdx, v)) {
        g[bestIdx] = v;
        count += countSolutions(g, maxCount - count);
        g[bestIdx] = 0;
      }
    }
    return count;
  }

  void generatePuzzle(int target) {
    memset(_given, 0, 81);
    if (!fillRandom(_given, 0)) return;
    uint8_t pos[81];
    for (int i = 0; i < 81; i++) pos[i] = i;
    for (int i = 80; i > 0; i--) {
      int j = random(i + 1);
      uint8_t t = pos[i]; pos[i] = pos[j]; pos[j] = t;
    }
    int givens = 81;
    uint8_t work[81];
    for (int i = 0; i < 81 && givens > target; i++) {
      int p = pos[i];
      uint8_t saved = _given[p];
      _given[p] = 0;
      memcpy(work, _given, 81);
      if (countSolutions(work, 2) != 1) {
        _given[p] = saved;
      } else {
        givens--;
      }
    }
    memcpy(_grid, _given, 81);
  }

  void newPuzzle() {
    _won = false;
    _badCell = -1;
    _row = _col = 0;
    generatePuzzle(targetGivens(_diff));
    for (int i = 0; i < 81; i++)
      if (!_given[i]) { _row = i / 9; _col = i % 9; break; }
    invalidate();
  }

  void checkWin() {
    for (int i = 0; i < 81; i++) if (!_grid[i]) return;
    for (int r = 0; r < 9; r++)
      for (int c = 0; c < 9; c++)
        if (!isValidPlacement(_grid, r * 9 + c, _grid[r * 9 + c], true)) return;
    _won = true;
  }
};

inline App* createSudoku() { return new SudokuApp(); }