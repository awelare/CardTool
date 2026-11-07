#pragma once
#include <stdint.h>
#include "input.h"

// Базовый класс приложения. Новое приложение = наследник + строка в registry.cpp
class App {
public:
  virtual ~App() {}
  virtual void start() {}                  // при входе
  virtual void stop() {}                   // при выходе
  virtual void onKey(const KeyEvent& e) = 0;
  virtual void update(uint32_t nowMs) {}   // каждый цикл (игры, таймеры)
  virtual void draw() = 0;                 // вызывается, когда invalidate()

  void invalidate() { _dirty = true; }
  bool takeDirty() { bool d = _dirty; _dirty = false; return d; }
  void requestExit() { _exit = true; }
  bool exitRequested() const { return _exit; }
protected:
  bool _dirty = true;
  bool _exit = false;
};

struct AppEntry {
  const char* title;   // название в списке
  const char* group;   // группа: строго одна из kGroups
  App* (*create)();
};
extern const AppEntry kApps[];
extern const int kAppsCount;
extern const char* const kGroups[];
extern const int kGroupsCount;
