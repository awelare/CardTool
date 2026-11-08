#pragma once
#include <string.h>
#include <stdio.h>
#include "../app.h"
#include "../ui.h"
#include "../config.h"

class SnakeApp : public App {
public:
  static constexpr int COLS = 30;
  static constexpr int ROWS = 19;
  static constexpr int CELL = 10;
  static constexpr int FX = 10;
  static constexpr int FY = 28;
  static constexpr int MAXLEN = COLS * ROWS;

  void start() override { reset(); }
  void stop() override {}

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
    // запрет разворота на 180° (сверяем с РЕАЛЬНЫМ текущим движением)
    if (_curDx == -dx && _curDy == -dy) return;
    _pendDx = dx; _pendDy = dy;
  }

  void update(uint32_t nowMs) override {
    if (_state != Playing) return;
    if ((uint32_t)(nowMs - _lastTick) < _tickMs) return;
    _lastTick = nowMs;
    step();
    invalidate();
  }

  void draw() override {
    gfx.startWrite();
    ui::titleBar("Змейка");

    if (_state == Over) {
      gfx.fillRect(FX - 1, FY - 1, COLS * CELL + 2, ROWS * CELL + 2, C_BG);
      drawCentered("Конец игры", C_ERR, FY + 60);
      char s[32]; snprintf(s, sizeof(s), "Счёт: %d", _score);
      drawCentered(s, C_FG, FY + 90);
      drawCentered("Enter — заново", C_DIM, FY + 120);
      _prevValid = false;
    } else {
      if (!_prevValid) {
        gfx.fillRect(FX - 1, FY - 1, COLS * CELL + 2, ROWS * CELL + 2, C_BG);
        memset(_prev, 0, sizeof(_prev));
      }
      for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
          uint8_t cur = _grid[y][x];
          if (cur == _prev[y][x]) continue;
          int px = FX + x * CELL, py = FY + y * CELL;
          uint16_t col;
          if (cur == 1) col = C_ACCENT;
          else if (cur == 2) col = C_ERR;
          else col = C_BG;
          gfx.fillRect(px + 1, py + 1, CELL - 2, CELL - 2, col);
          _prev[y][x] = cur;
        }
      }
      _prevValid = true;
      gfx.drawRect(FX - 1, FY - 1, COLS * CELL + 2, ROWS * CELL + 2, C_DIM);
    }

    char buf[56];
    if (_state == Over)
      snprintf(buf, sizeof(buf), "Счёт: %d   Enter — заново   Esc — выход", _score);
    else
      snprintf(buf, sizeof(buf), "Счёт: %d   ; . , / — ход   Esc — выход", _score);
    ui::footer(buf);
    gfx.endWrite();
  }

private:
  enum State : uint8_t { Playing, Over };
  State _state = Playing;
  int _score = 0;
  uint32_t _tickMs = 180;
  uint32_t _lastTick = 0;
  int8_t _curDx = 1, _curDy = 0;   // реальное направление (обновляется на тике)
  int8_t _pendDx = 1, _pendDy = 0; // буфер нажатий
  uint8_t _sx[MAXLEN], _sy[MAXLEN];
  int _head = 0, _tail = 0, _len = 3;
  int8_t _foodX = -1, _foodY = -1;
  uint8_t _grid[ROWS][COLS];
  uint8_t _prev[ROWS][COLS];
  bool _prevValid = false;

  void drawCentered(const char* s, uint16_t color, int y) {
    int w = (int)strlen(s) * 6;
    int x = FX + (COLS * CELL - w) / 2;
    if (x < FX + 4) x = FX + 4;
    gfx.setTextColor(color, C_BG);
    gfx.drawString(s, x, y);
  }

  void reset() {
    _state = Playing;
    _score = 0;
    _tickMs = 180;
    _curDx = 1; _curDy = 0;
    _pendDx = 1; _pendDy = 0;
    _len = 3;
    _tail = 0;
    _head = 2;
    _sx[0] = 5; _sy[0] = 9;
    _sx[1] = 6; _sy[1] = 9;
    _sx[2] = 7; _sy[2] = 9;
    _prevValid = false;
    if (!placeFood()) { _foodX = 0; _foodY = 0; }
    rebuildGrid();
    _lastTick = millis();
    invalidate();
  }

  bool onSnake(int x, int y) const {
    for (int i = 0; i < _len; i++) {
      int idx = (_tail + i) % MAXLEN;
      if (_sx[idx] == x && _sy[idx] == y) return true;
    }
    return false;
  }

  bool placeFood() {
    for (int t = 0; t < 800; t++) {
      int x = (int)random(COLS);
      int y = (int)random(ROWS);
      if (!onSnake(x, y)) { _foodX = x; _foodY = y; return true; }
    }
    return false;
  }

  void step() {
    _curDx = _pendDx; _curDy = _pendDy;
    int hx = _sx[_head], hy = _sy[_head];
    int nx = hx + _curDx, ny = hy + _curDy;
    if (nx < 0 || ny < 0 || nx >= COLS || ny >= ROWS) { _state = Over; return; }
    bool eat = (nx == _foodX && ny == _foodY);
    int start = eat ? 0 : 1;  // при движении хвост освобождается — его пропускаем
    for (int i = start; i < _len; i++) {
      int idx = (_tail + i) % MAXLEN;
      if (_sx[idx] == nx && _sy[idx] == ny) { _state = Over; return; }
    }
    if (eat) {
      if (_len < MAXLEN) {
        _head = (_head + 1) % MAXLEN;
        _sx[_head] = nx; _sy[_head] = ny;
        _len++;
      }
      _score++;
      if (_tickMs > 70) _tickMs -= 6;
      if (!placeFood()) { _state = Over; return; }
    } else {
      _head = (_head + 1) % MAXLEN;
      _sx[_head] = nx; _sy[_head] = ny;
      _tail = (_tail + 1) % MAXLEN;
    }
    rebuildGrid();
  }

  void rebuildGrid() {
    memset(_grid, 0, sizeof(_grid));
    for (int i = 0; i < _len; i++) {
      int idx = (_tail + i) % MAXLEN;
      _grid[_sy[idx]][_sx[idx]] = 1;
    }
    if (_foodX >= 0 && _foodY >= 0) _grid[_foodY][_foodX] = 2;
  }
};

inline App* createSnake() { return new SnakeApp(); }