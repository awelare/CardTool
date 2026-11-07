#include "app.h"
#include "apps/hwtest.h"
#include "apps/template.h"
#include "apps/settings.h"

// Группы (порядок = порядок в лаунчере). Пустые группы не показываются.
const char* const kGroups[] = {"Учёба", "Игры", "Хакинг", "Система"};
const int kGroupsCount = sizeof(kGroups) / sizeof(kGroups[0]);

// Приложения: {"Название", "Группа", функция_создания},
const AppEntry kApps[] = {
  {"Тест железа",       "Система", createHwTest},
  {"Шаблон приложения", "Система", createTemplate},
  {"Настройки",         "Система", createSettings},
};
const int kAppsCount = sizeof(kApps) / sizeof(kApps[0]);
