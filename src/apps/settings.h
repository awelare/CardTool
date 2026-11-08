#pragma once
#include <stdio.h>
#include <M5Cardputer.h>
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
    if (up && _row > 0)         { _row--; invalidate(); return; }
    if (dn && _row < ROWS - 1)  { _row++; invalidate(); return; }
    int d = 0;
    if (lf) d = -1;
    if (rt || e.key == Key::ENTER) d = +1;
    if (d != 0) change(d);
  }

  void draw() override {
    gfx.startWrite();
    gfx.fillScreen(C_BG);
    ui::titleBar("Настройки");
    char v[40];
    static const char* labels[ROWS] = {
      "Тема", "Акцент", "Поворот", "Инверсия", "Громкость", "Язык", "Автоповтор",
      "Автосон экрана", "Сбросить всё"
    };
    for (int i = 0; i < ROWS; i++) {
      int y = ui::LIST_TOP + i * ui::ROW_H;
      switch (i) {
        case 0: snprintf(v, sizeof(v), "< %s >", themeName(g_cfg.theme)); break;
        case 1: snprintf(v, sizeof(v), "< %s >", accentName(g_cfg.accent)); break;
        case 2: snprintf(v, sizeof(v), "< %d >", g_cfg.rotation); break;
        case 3: snprintf(v, sizeof(v), "< %s >", g_cfg.invert ? "вкл" : "выкл"); break;
        case 4: snprintf(v, sizeof(v), g_cfg.volume == 0 ? "< выкл >" : "< %d%% >", g_cfg.volume); break;
        case 5: snprintf(v, sizeof(v), "< %s >", g_cfg.language == 0 ? "RU" : "EN"); break;
        case 6: snprintf(v, sizeof(v), "< %s >", repeatName(g_cfg.keyRepeat)); break;
        case 7: snprintf(v, sizeof(v), "< %s >", autoOffName(g_cfg.autoOffSec)); break;
        case 8: snprintf(v, sizeof(v), "< Enter >"); break;
        default: snprintf(v, sizeof(v), "Enter"); break;
      }
      ui::row(y, labels[i], v, i == _row);
      if (i == 1)
        gfx.fillRect(SCREEN_W - 8 - gfx.textWidth(v) - 22, y + 3, 16, 16, C_ACCENT);
    }
    ui::footer("Fn+; Fn+. строка Fn+, Fn+/ знач.");
    gfx.endWrite();
  }

private:
  static const int ROWS = 9;
  int _row = 0;

  static const char* repeatName(uint8_t r) {
    static const char* n[] = {"выкл", "медл", "средн", "быстро"};
    return n[r > 3 ? 0 : r];
  }

  static const char* autoOffName(uint16_t s) {
    switch (s) {
      case 0:   return "выкл";
      case 30:  return "30 с";
      case 60:  return "1 мин";
      case 120: return "2 мин";
      case 300: return "5 мин";
      default:  return "выкл";
    }
  }

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
      case 4: {
        int v = g_cfg.volume + d * 5;
        if (v < 0)   v = 0;
        if (v > 100) v = 100;
        g_cfg.volume = v;
        M5.Speaker.setVolume(v * 255 / 100);
        if (v > 0) M5.Speaker.tone(1000, 30);
        break;
      }
      case 5: g_cfg.language = (g_cfg.language + 1) % 2; break;
      case 6: g_cfg.keyRepeat = (g_cfg.keyRepeat + d + 4) % 4; break;
      case 7: {
        const uint16_t steps[] = {0, 30, 60, 120, 300};
        int k = 0;
        for (int i = 0; i < 5; i++) if (steps[i] == g_cfg.autoOffSec) k = i;
        k = (k + d + 5) % 5;
        g_cfg.autoOffSec = steps[k];
        break;
      }
      case 8: themeReset(); break;
      default: break;
    }
    themeApply();
    themeApplyToDisplay();
    themeSave();
    invalidate();
  }
};

inline App* createSettings() { return new SettingsApp(); }