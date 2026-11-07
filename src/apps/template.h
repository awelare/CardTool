#pragma once
#include <stdio.h>
#include "../app.h"
#include "../ui.h"

// ШАБЛОН: скопируй файл, переименуй класс и функцию create..., добавь в registry.cpp
class TemplateApp : public App {
public:
  void start() override { _count = 0; }          // инициализация при входе
  void onKey(const KeyEvent& e) override {       // ввод
    if (e.key == Key::ESC) { requestExit(); return; }
    if (e.key == Key::ENTER) { _count++; invalidate(); }
  }
  void update(uint32_t nowMs) override {}        // логика/таймеры
  void draw() override {                         // отрисовка (только по invalidate)
    gfx.startWrite();
    gfx.fillScreen(C_BG);
    ui::titleBar("Шаблон приложения");
    char buf[40];
    snprintf(buf, sizeof(buf), "Enter нажат: %d", _count);
    gfx.setTextColor(C_FG, C_BG);
    gfx.drawString(buf, ui::PAD, ui::LIST_TOP + 8);
    ui::footer("Enter +1   Fn+` выход");
    gfx.endWrite();
  }
private:
  int _count = 0;
};
inline App* createTemplate() { return new TemplateApp(); }
