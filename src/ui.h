#pragma once
// Единый стиль интерфейса. ВСЕ приложения рисуют через эти функции и константы.
#include "display.h"

namespace ui {
constexpr int TITLE_H = 24;   // высота заголовка
constexpr int ROW_H   = 22;   // высота строки списка
constexpr int LIST_TOP = 28;  // начало списка
constexpr int FOOT_Y  = 222;  // строка подсказки внизу
constexpr int PAD     = 10;   // левый отступ текста в строке

inline int visibleRows() { return (FOOT_Y - LIST_TOP) / ROW_H; }

// Заголовок: полоса акцентного цвета, белый текст
inline void titleBar(const char* text) {
  gfx.setFont(UI_FONT);
  gfx.fillRect(0, 0, SCREEN_W, TITLE_H, C_ACCENT);
  gfx.setTextColor(RGB565(255,255,255), C_ACCENT);
  gfx.drawString(text, 6, 4);
}

// Подсказка по клавишам внизу экрана (короткая!)
inline void footer(const char* hint) {
  gfx.setFont(UI_FONT);
  gfx.fillRect(0, FOOT_Y, SCREEN_W, 18, C_BG);
  gfx.setTextColor(C_DIM, C_BG);
  gfx.drawString(hint, 4, FOOT_Y);
}

// Строка списка: слева название, справа значение (серым). Выбранная подсвечена C_SEL
inline void row(int y, const char* label, const char* value, bool selected) {
  uint16_t bg = selected ? C_SEL : C_BG;
  gfx.setFont(UI_FONT);
  gfx.fillRect(0, y, SCREEN_W, ROW_H, bg);
  gfx.setTextColor(selected ? C_FG : C_DIM, bg);
  gfx.drawString(label, PAD, y + 3);
  if (value && value[0]) {
    gfx.setTextColor(C_DIM, bg);
    gfx.drawString(value, SCREEN_W - 8 - gfx.textWidth(value), y + 3);
  }
}
}  // namespace ui
