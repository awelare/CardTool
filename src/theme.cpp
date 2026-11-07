#include <Preferences.h>
#include "theme.h"
#include "display.h"

UiConfig g_cfg;

struct Preset { const char* name; uint16_t bg, fg, dim, sel; };
static const Preset kThemes[] = {
  {"Тёмная",  RGB565(0,0,0),       RGB565(255,255,255), RGB565(140,140,140), RGB565(40,40,60)},
  {"Светлая", RGB565(245,245,245), RGB565(20,20,20),    RGB565(110,110,110), RGB565(205,215,235)},
  {"Синяя",   RGB565(0,20,50),     RGB565(230,240,255), RGB565(120,150,190), RGB565(0,50,100)},
  {"Янтарь",  RGB565(15,8,0),      RGB565(255,176,0),   RGB565(150,100,0),   RGB565(55,32,0)},
};
struct Accent { const char* name; uint16_t c; };
static const Accent kAccents[] = {
  {"Синий",      RGB565(0,120,215)},
  {"Зелёный",    RGB565(0,150,70)},
  {"Красный",    RGB565(200,40,40)},
  {"Оранжевый",  RGB565(220,110,0)},
  {"Фиолетовый", RGB565(130,60,190)},
  {"Бирюзовый",  RGB565(0,150,150)},
  {"Розовый",    RGB565(210,70,150)},
  {"Серый",      RGB565(90,90,100)},
};
const int kThemeCount  = sizeof(kThemes) / sizeof(kThemes[0]);
const int kAccentCount = sizeof(kAccents) / sizeof(kAccents[0]);
const char* themeName(int i)  { return kThemes[i].name; }
const char* accentName(int i) { return kAccents[i].name; }
uint16_t accentColor(int i)   { return kAccents[i].c; }

void themeApply() {
  const Preset& t = kThemes[g_cfg.theme];
  g_cfg.bg = t.bg; g_cfg.fg = t.fg; g_cfg.dim = t.dim; g_cfg.sel = t.sel;
  g_cfg.accent_c = kAccents[g_cfg.accent].c;
}

void themeReset() {
  g_cfg.theme = 0; g_cfg.accent = 0; g_cfg.rotation = 7; g_cfg.invert = 0;
  themeApply();
}

void themeLoad() {
  themeReset();
  Preferences p;
  if (!p.begin("ui", false)) return;
  g_cfg.theme    = p.getUChar("theme", g_cfg.theme);
  g_cfg.accent   = p.getUChar("accent", g_cfg.accent);
  g_cfg.rotation = p.getUChar("rot", g_cfg.rotation);
  g_cfg.invert   = p.getUChar("inv", g_cfg.invert);
  p.end();
  if (g_cfg.theme >= kThemeCount) g_cfg.theme = 0;
  if (g_cfg.accent >= kAccentCount) g_cfg.accent = 0;
  if (!(g_cfg.rotation == 1 || g_cfg.rotation == 3 || g_cfg.rotation == 5 || g_cfg.rotation == 7)) g_cfg.rotation = 7;
  g_cfg.invert = g_cfg.invert ? 1 : 0;
  themeApply();
}

void themeSave() {
  Preferences p;
  if (!p.begin("ui", false)) return;
  p.putUChar("theme", g_cfg.theme);
  p.putUChar("accent", g_cfg.accent);
  p.putUChar("rot", g_cfg.rotation);
  p.putUChar("inv", g_cfg.invert);
  p.end();
}

void themeApplyToDisplay() {
  gfx.setRotation(g_cfg.rotation);
  gfx.invertDisplay(g_cfg.invert != 0);
}
