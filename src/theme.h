#pragma once
#include "config.h"

extern const int kThemeCount;
extern const int kAccentCount;
const char* themeName(int i);
const char* accentName(int i);
uint16_t accentColor(int i);

void themeLoad();            // прочитать настройки (до gfx.init)
void themeSave();            // сохранить
void themeApply();           // пересчитать цвета из номера темы/акцента
void themeReset();           // значения по умолчанию
void themeApplyToDisplay();  // поворот + инверсия -> экран (после gfx.init)
