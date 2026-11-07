#pragma once
#include <string.h>
#include <stdio.h>
#include "app.h"
#include "ui.h"

// Два уровня: список групп -> список приложений группы
class Launcher : public App {
public:
  int chosen = -1;   // индекс приложения в kApps, main его забирает

  void start() override { _level = 0; _sel = 0; _first = 0; build(); invalidate(); }

  void onKey(const KeyEvent& e) override {
    bool up = e.key == Key::UP   || (e.key == Key::CHAR && e.ch == ';');
    bool dn = e.key == Key::DOWN || (e.key == Key::CHAR && e.ch == '.');
    if (up && _sel > 0) { _sel--; invalidate(); }
    if (dn && _sel < _n - 1) { _sel++; invalidate(); }
    if (e.key == Key::ENTER && _n > 0) {
      if (_level == 0) {
        _group = _list[_sel]; _savedSel = _sel;
        _level = 1; _sel = 0; _first = 0; build(); invalidate();
      } else {
        chosen = _list[_sel];
      }
    }
    if (e.key == Key::ESC && _level == 1) {
      _level = 0; _sel = _savedSel; _first = 0; build(); invalidate();
    }
  }

  void draw() override {
    int vis = ui::visibleRows();
    if (_sel < _first) _first = _sel;
    if (_sel >= _first + vis) _first = _sel - vis + 1;
    gfx.startWrite();
    ui::titleBar(_level == 0 ? "Cardputer OS" : kGroups[_group]);
    char v[8];
    for (int i = 0; i < vis; i++) {
      int idx = _first + i;
      int y = ui::LIST_TOP + i * ui::ROW_H;
      if (idx < _n) {
        if (_level == 0) {
          snprintf(v, sizeof(v), "%d", countIn(_list[idx]));
          ui::row(y, kGroups[_list[idx]], v, idx == _sel);
        } else {
          ui::row(y, kApps[_list[idx]].title, "", idx == _sel);
        }
      } else {
        gfx.fillRect(0, y, SCREEN_W, ui::ROW_H, C_BG);
      }
    }
    ui::footer(_level == 0 ? "Fn+; Fn+. выбор  Enter вход" : "Enter запуск  Fn+` назад");
    gfx.endWrite();
  }

private:
  static const int MAXN = 64;
  int _list[MAXN];
  int _n = 0, _level = 0, _sel = 0, _first = 0, _group = 0, _savedSel = 0;

  int countIn(int g) const {
    int c = 0;
    for (int i = 0; i < kAppsCount; i++)
      if (strcmp(kApps[i].group, kGroups[g]) == 0) c++;
    return c;
  }
  void build() {
    _n = 0;
    if (_level == 0) {
      for (int g = 0; g < kGroupsCount; g++)
        if (countIn(g) > 0 && _n < MAXN) _list[_n++] = g;
    } else {
      for (int i = 0; i < kAppsCount; i++)
        if (strcmp(kApps[i].group, kGroups[_group]) == 0 && _n < MAXN) _list[_n++] = i;
    }
  }
};
