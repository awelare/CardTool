#pragma once
#include <stdio.h>
#include <string.h>
#include "../app.h"
#include "../ui.h"

class Game2048App : public App {
public:
  void start() override { reset(); }

  void onKey(const KeyEvent& e) override {
    if (e.key == Key::ESC) { requestExit(); return; }
    if (_state == Over) {
      if (e.key == Key::ENTER) reset();
      return;
    }
    int dx = 0, dy = 0;
    switch (e.key) {
      case Key::UP:    dy = -1; break;
      case Key::DOWN:  dy =  1; break;
      case Key::LEFT:  dx = -1; break;
      case Key::RIGHT: dx =  1; break;
      case Key::CHAR:
        if (e.ch == ';') dy = -1;
        else if (e.ch == '.') dy = 1;
        else if (e.ch == ',') dx = -1;
        else if (e.ch == '/') dx = 1;
        break;
      default: break;
    }
    if (!dx && !dy) return;
    int gain = 0;
    if (moveGrid(dx, dy, gain)) {
      _score += gain;
      spawn();
      checkWin();
      if (!hasMoves()) _state = Over;
      invalidate();
    }
  }

  void draw() override {
    gfx.startWrite();

    char t[48];
    snprintf(t, sizeof(t), "2048%s  Счёт: %d",
             _reached2048 ? "!" : " ", _score);
    ui::titleBar(t);

    drawBoard();

    if (_state == Over) {
      int bh = 44;
      int by = BY + (BH - bh) / 2;
      gfx.fillRect(BX, by, BW, bh, 0x2104);
      gfx.drawRect(BX, by, BW, bh, C_ERR);
      const char* m1 = "Конец игры";
      int w1 = gfx.textWidth(m1);
      gfx.setTextColor(C_ERR, 0x2104);
      gfx.drawString(m1, BX + (BW - w1) / 2, by + 6);
      const char* m2 = "Enter — заново";
      int w2 = gfx.textWidth(m2);
      gfx.setTextColor(C_DIM, 0x2104);
      gfx.drawString(m2, BX + (BW - w2) / 2, by + 24);
      ui::footer("Конец!  Enter — заново   Esc — выход");
    } else {
      ui::footer("; . , / — ход   Esc — выход");
    }

    gfx.endWrite();
  }

private:
  enum State : uint8_t { Playing, Over };
  State _state = Playing;
  int _score = 0;
  bool _reached2048 = false;
  uint32_t _grid[4][4];

  static constexpr int CELL = 42;
  static constexpr int GAP  = 4;
  static constexpr int BX   = 70;
  static constexpr int BY   = 32;
  static constexpr int BW   = 4 * CELL + 3 * GAP;
  static constexpr int BH   = BW;

  void reset() {
    memset(_grid, 0, sizeof(_grid));
    _score = 0;
    _state = Playing;
    _reached2048 = false;
    spawn(); spawn();
    invalidate();
  }

  void spawn() {
    int cells[16][2];
    int n = 0;
    for (int r = 0; r < 4; r++)
      for (int c = 0; c < 4; c++)
        if (!_grid[r][c]) { cells[n][0] = r; cells[n][1] = c; n++; }
    if (!n) return;
    int k = (int)random(n);
    _grid[cells[k][0]][cells[k][1]] = (random(10) == 0) ? 4 : 2;
  }

  bool hasMoves() const {
    for (int r = 0; r < 4; r++)
      for (int c = 0; c < 4; c++) {
        if (!_grid[r][c]) return true;
        if (c < 3 && _grid[r][c] == _grid[r][c+1]) return true;
        if (r < 3 && _grid[r][c] == _grid[r+1][c]) return true;
      }
    return false;
  }

  void checkWin() {
    if (_reached2048) return;
    for (int r = 0; r < 4; r++)
      for (int c = 0; c < 4; c++)
        if (_grid[r][c] == 2048) { _reached2048 = true; return; }
  }

  // сжимает/сливает одну линию из 4 клеток к индексу 0
  bool slideLine(uint32_t* line, int& gain) {
    uint32_t tmp[4] = {0,0,0,0};
    int j = 0;
    for (int i = 0; i < 4; i++) if (line[i]) tmp[j++] = line[i];
    for (int i = 0; i < 3; i++) {
      if (tmp[i] && tmp[i] == tmp[i+1]) {
        tmp[i] *= 2;
        gain += (int)tmp[i];
        tmp[i+1] = 0;
        i++;
      }
    }
    uint32_t out[4] = {0,0,0,0};
    j = 0;
    for (int i = 0; i < 4; i++) if (tmp[i]) out[j++] = tmp[i];
    bool moved = false;
    for (int i = 0; i < 4; i++) if (line[i] != out[i]) moved = true;
    memcpy(line, out, sizeof(out));
    return moved;
  }

  bool moveGrid(int dx, int dy, int& gain) {
    bool moved = false;
    if (dx) {
      for (int r = 0; r < 4; r++) {
        uint32_t line[4];
        int idx[4];
        for (int i = 0; i < 4; i++) {
          int c = (dx < 0) ? i : (3 - i);
          line[i] = _grid[r][c];
          idx[i] = c;
        }
        if (slideLine(line, gain)) moved = true;
        for (int i = 0; i < 4; i++) _grid[r][idx[i]] = line[i];
      }
    } else {
      for (int c = 0; c < 4; c++) {
        uint32_t line[4];
        int idx[4];
        for (int i = 0; i < 4; i++) {
          int r = (dy < 0) ? i : (3 - i);
          line[i] = _grid[r][c];
          idx[i] = r;
        }
        if (slideLine(line, gain)) moved = true;
        for (int i = 0; i < 4; i++) _grid[idx[i]][c] = line[i];
      }
    }
    return moved;
  }

  uint16_t tileBg(uint32_t v) const {
    switch (v) {
      case 0:    return 0x3186;
      case 2:    return 0xE71C;
      case 4:    return 0xDEFB;
      case 8:    return 0xFD20;
      case 16:   return 0xFB40;
      case 32:   return 0xF980;
      case 64:   return 0xF100;
      case 128:  return 0xFFE0;
      case 256:  return 0xFEA0;
      case 512:  return 0xFE60;
      case 1024: return 0xFDE0;
      case 2048: return 0xFE00;
      default:   return 0xF800;
    }
  }
  uint16_t tileFg(uint32_t v) const {
    if (!v) return 0x3186;
    return (v <= 4) ? 0x4208 : 0xFFFF;
  }

  void drawBoard() {
    gfx.fillRect(BX, BY, BW, BH, 0x2104);
    for (int r = 0; r < 4; r++)
      for (int c = 0; c < 4; c++) {
        int x = BX + c * (CELL + GAP);
        int y = BY + r * (CELL + GAP);
        uint32_t v = _grid[r][c];
        uint16_t bg = tileBg(v);
        gfx.fillRect(x, y, CELL, CELL, bg);
        if (v) {
          char buf[8];
          snprintf(buf, sizeof(buf), "%lu", (unsigned long)v);
          int tw = gfx.textWidth(buf);
          int th = gfx.fontHeight();
          gfx.setTextColor(tileFg(v), bg);
          gfx.drawString(buf, x + (CELL - tw) / 2, y + (CELL - th) / 2);
        }
      }
  }
};

inline App* create2048() { return new Game2048App(); }