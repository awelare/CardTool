#include <M5Cardputer.h>
#include "display.h"
#include "theme.h"
#include "storage.h"
#include "input.h"
#include "app.h"
#include "launcher.h"

LGFX_ILI9341 gfx;
bool g_sdOk = false;

static Launcher launcher;
static App* cur = nullptr;

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);          // клавиатура TCA8418
  M5Cardputer.Display.setBrightness(0);  // встроенный экран гасим

  themeLoad();
  gfx.init();
  themeApplyToDisplay();
  gfx.fillScreen(C_BG);

  storageBegin();
  launcher.start();
}

void loop() {
  KeyEvent e;
  App* a = cur ? cur : &launcher;
  if (inputPoll(e)) a->onKey(e);
  a->update(millis());

  if (cur && cur->exitRequested()) {
    cur->stop();
    delete cur;
    cur = nullptr;
    launcher.invalidate();
    gfx.fillScreen(C_BG);
    a = &launcher;
  } else if (!cur && launcher.chosen >= 0) {
    cur = kApps[launcher.chosen].create();
    launcher.chosen = -1;
    gfx.fillScreen(C_BG);
    cur->start();
    a = cur;
  }

  if (a->takeDirty()) a->draw();

  ui::updateBattery();
  delay(2);
}
