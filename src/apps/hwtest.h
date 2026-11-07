#pragma once
#include <stdio.h>
#include "../app.h"
#include "../display.h"
#include "../storage.h"

class HwTestApp : public App {
public:
  void onKey(const KeyEvent& e) override {
    if (e.key == Key::ESC) { requestExit(); return; }
    const char* n = "?";
    switch (e.key) {
      case Key::UP: n = "UP"; break;
      case Key::DOWN: n = "DOWN"; break;
      case Key::LEFT: n = "LEFT"; break;
      case Key::RIGHT: n = "RIGHT"; break;
      case Key::ENTER: n = "ENTER"; break;
      case Key::DEL: n = "DEL"; break;
      case Key::CHAR: n = "CHAR"; break;
      default: break;
    }
    if (e.key == Key::CHAR) snprintf(_last, sizeof(_last), "%s '%c'", n, e.ch);
    else snprintf(_last, sizeof(_last), "%s", n);
    invalidate();
  }
  void draw() override {
    gfx.startWrite();
    gfx.fillScreen(C_BG);
    // цветные полосы: проверка цветов/инверсии (К З С Ж Г Ч Б Ч)
    const uint16_t cols[8] = {
      RGB565(255,0,0), RGB565(0,255,0), RGB565(0,0,255), RGB565(255,255,0),
      RGB565(0,255,255), RGB565(255,0,255), RGB565(255,255,255), RGB565(90,90,90)};
    for (int i = 0; i < 8; i++) gfx.fillRect(i * 40, 0, 40, 30, cols[i]);
    gfx.setFont(UI_FONT);
    gfx.setTextColor(C_FG, C_BG);
    gfx.drawString("Привет, мир! АБВГДЕЁЖЗ", 4, 38);
    gfx.drawString("абвгдеёжзийклмнопрстуфхцчшщъыьэюя", 4, 58);
    char buf[64];
    snprintf(buf, sizeof(buf), "Ширина Ж=%d  W=%d px", gfx.textWidth("Ж"), gfx.textWidth("W"));
    gfx.drawString(buf, 4, 80);
    if (g_sdOk) {
      snprintf(buf, sizeof(buf), "SD: OK, %d МБ", (int)(SD.cardSize() / (1024 * 1024)));
      gfx.setTextColor(C_OK, C_BG);
    } else {
      snprintf(buf, sizeof(buf), "SD: нет карты / ошибка");
      gfx.setTextColor(C_ERR, C_BG);
    }
    gfx.drawString(buf, 4, 104);
    gfx.setTextColor(C_FG, C_BG);
    snprintf(buf, sizeof(buf), "Клавиша: %s", _last);
    gfx.drawString(buf, 4, 128);
    snprintf(buf, sizeof(buf), "Свободно ОЗУ: %u КБ", (unsigned)(ESP.getFreeHeap() / 1024));
    gfx.drawString(buf, 4, 152);
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString("Жми клавиши. Fn+` выход", 4, 220);
    gfx.endWrite();
  }
private:
  char _last[24] = "-";
};
inline App* createHwTest() { return new HwTestApp(); }
