#include "app.h"
#include "apps/hwtest.h"
#include "apps/template.h"
#include "apps/settings.h"
#include "apps/irremote.h"
#include "apps/calculator.h"
#include "apps/units.h"
#include "apps/bases.h"
#include "apps/progcalc.h"
#include "apps/musicplayer.h"
#include "apps/videoplayer.h"
#include "apps/snake.h"
#include "apps/game2048.h"
#include "apps/sudoku.h"
#include "apps/minesweeper.h"
#include "apps/filemanager.h"
#include "apps/refbook.h"
#include "apps/periodic.h"
#include "apps/aichat.h"

// Группы (порядок = порядок в лаунчере). Пустые группы не показываются.
const char* const kGroups[] = {"Учёба", "Игры", "Инструменты", "Медиа", "Хакинг", "Система"};
const int kGroupsCount = sizeof(kGroups) / sizeof(kGroups[0]);

// Приложения: {"Название", "Группа", функция_создания},
const AppEntry kApps[] = {
  {"Тест железа",       "Система", createHwTest},
  {"Шаблон приложения", "Система", createTemplate},
  {"Настройки",         "Система", createSettings},
  {"ИК-пульт", "Хакинг", createIrRemote},
  {"Калькулятор",            "Учёба", createCalculator},
  {"Конвертер единиц",       "Учёба", createUnits},
  {"Системы счисления",      "Учёба", createBases},
  {"Математика в разных СС", "Учёба", createProgCalc},
  {"Музыка", "Медиа", createMusic},
  {"Видео",  "Медиа", createVideo},
  {"Змейка", "Игры", createSnake},
  {"2048", "Игры", create2048},
  {"Судоку", "Игры", createSudoku},
  {"Сапёр", "Игры", createMinesweeper},
  {"Файлы", "Система", createFileManager},
{"Формулы", "Учёба", createFormulas},
{"Определения", "Учёба", createDefinitions},
{"Таблица Менделеева", "Учёба", createPeriodic},
{"Спросить ИИ", "Инструменты", createAiChat},
};
const int kAppsCount = sizeof(kApps) / sizeof(kApps[0]);
