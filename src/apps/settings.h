#pragma once
#include <stdio.h>
#include "../app.h"
#include "../ui.h"
#include "../theme.h"

class SettingsApp : public App {
public:
  void onKey(const KeyEvent& e) override {
    bool up = e.key == Key::UP    || (e.key == Key::CHAR && e.ch == ';');
    bool dn = e.key == Key::DOWN  || (e.key == Key::CHAR && e.ch == '.');
    bool lf = e.key == Key::LEFT  || (e.key == Key::CHAR && e.ch == ',');
    bool rt = e.key == Key::RIGHT || (e.key == Key::CHAR && e.ch == '/');
    if (e.key == Key::ESC) { requestExit(); return; }
    if (up && _row > 0) { _row--; invalidate(); }
    if (dn && _row < ROWS - 1) { _row++; invalidate(); }
    if (lf) change(-1);
    if (rt || e.key == Key::ENTER) change(+1);
  }

  void draw() override {
    gfx.startWrite();
    gfx.fillScreen(C_BG);
    ui::titleBar("Настройки");
    char v[40];
    static const char* labels[ROWS] = {"Тема", "Акцентный цвет", "Поворот экрана", "Инверсия цветов", "Сбросить всё"};
    for (int i = 0; i < ROWS; i++) {
      int y = ui::LIST_TOP + i * ui::ROW_H;
      switch (i) {
        case 0: snprintf(v, sizeof(v), "< %s >", themeName(g_cfg.theme)); break;
        case 1: snprintf(v, sizeof(v), "< %s >", accentName(g_cfg.accent)); break;
        case 2: snprintf(v, sizeof(v), "< %d >", g_cfg.rotation); break;
        case 3: snprintf(v, sizeof(v), "< %s >", g_cfg.invert ? "вкл" : "выкл"); break;
        default: snprintf(v, sizeof(v), "Enter"); break;
      }
      ui::row(y, labels[i], v, i == _row);
      if (i == 1) gfx.fillRect(SCREEN_W - 8 - gfx.textWidth(v) - 22, y + 3, 16, 16, C_ACCENT);
    }
    ui::footer("Fn+; Fn+. строка Fn+, Fn+/ знач.");
    gfx.endWrite();
  }

private:
  static const int ROWS = 5;
  int _row = 0;

  void change(int d) {
    switch (_row) {
      case 0: g_cfg.theme  = (g_cfg.theme  + d + kThemeCount)  % kThemeCount;  break;
      case 1: g_cfg.accent = (g_cfg.accent + d + kAccentCount) % kAccentCount; break;
      case 2: {
        const uint8_t r[4] = {1, 3, 5, 7};
        int k = 0;
        for (int i = 0; i < 4; i++) if (r[i] == g_cfg.rotation) k = i;
        g_cfg.rotation = r[(k + d + 4) % 4];
        break;
      }
      case 3: g_cfg.invert = !g_cfg.invert; break;
      default: themeReset(); break;
    }
    themeApply();
    themeApplyToDisplay();
    themeSave();
    invalidate();
  }
};
inline App* createSettings() { return new SettingsApp(); }
