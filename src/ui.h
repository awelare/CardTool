#pragma once
#include <M5Cardputer.h>
#include "display.h"

// Единый стиль интерфейса.
namespace ui {

constexpr int TITLE_H = 24;
constexpr int ROW_H = 22;
constexpr int LIST_TOP = 28;
constexpr int FOOT_Y = 222;
constexpr int PAD = 10;

inline int visibleRows() {
  return (FOOT_Y - LIST_TOP) / ROW_H;
}

// Индикатор заряда батареи справа в заголовке.
inline void drawBattery() {
  int level = M5Cardputer.Power.getBatteryLevel();
  level = constrain(level, 0, 100);

  char value[8];
  snprintf(value, sizeof(value), "%d%%", level);

  gfx.setFont(UI_FONT);
  gfx.setTextColor(RGB565(255, 255, 255), C_ACCENT);
  gfx.drawString(value, 250, 4);

  // Контур батареи и контакт.
  gfx.drawRect(290, 6, 23, 13, RGB565(255, 255, 255));
  gfx.fillRect(313, 10, 3, 5, RGB565(255, 255, 255));

  // Заполнение в соответствии с зарядом.
  int fillWidth = 19 * level / 100;
  if (fillWidth > 0) {
    gfx.fillRect(292, 8, fillWidth, 9, RGB565(255, 255, 255));
  }
}

// Заголовок: полоса акцентного цвета, белый текст и заряд батареи.
inline void titleBar(const char* text) {
  gfx.setFont(UI_FONT);
  gfx.fillRect(0, 0, SCREEN_W, TITLE_H, C_ACCENT);
  gfx.setTextColor(RGB565(255, 255, 255), C_ACCENT);
  gfx.drawString(text, 6, 4);
  drawBattery();
}

// Обновляет только область индикатора, примерно раз в секунду.
inline void updateBattery() {
  static uint32_t lastUpdate = 0;
  const uint32_t now = millis();

  if (now - lastUpdate < 1000) return;
  lastUpdate = now;

  gfx.startWrite();
  gfx.fillRect(248, 2, 70, 20, C_ACCENT);
  drawBattery();
  gfx.endWrite();
}

// Подсказка по клавишам внизу экрана.
inline void footer(const char* hint) {
  gfx.setFont(UI_FONT);
  gfx.fillRect(0, FOOT_Y, SCREEN_W, 18, C_BG);
  gfx.setTextColor(C_DIM, C_BG);
  gfx.drawString(hint, 4, FOOT_Y);
}

// Строка списка.
inline void row(int y, const char* label,
                const char* value, bool selected) {
  uint16_t bg = selected ? C_SEL : C_BG;

  gfx.setFont(UI_FONT);
  gfx.fillRect(0, y, SCREEN_W, ROW_H, bg);
  gfx.setTextColor(selected ? C_FG : C_DIM, bg);
  gfx.drawString(label, PAD, y + 3);

  if (value && value[0]) {
    gfx.setTextColor(C_DIM, bg);
    gfx.drawString(value,
                   SCREEN_W - 8 - gfx.textWidth(value),
                   y + 3);
  }
}

} // namespace ui
