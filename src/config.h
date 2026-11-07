#pragma once
#include <stdint.h>
// ---- Пины внешнего ILI9341 (разъём EXT) ----
#define PIN_TFT_CS   5
#define PIN_TFT_RST  3
#define PIN_TFT_DC   6
#define PIN_TFT_MOSI 14   // если глючит SD/экран: 13
#define PIN_TFT_SCK  40   // если глючит SD/экран: 15
// ---- SD (общая шина с экраном) ----
#define PIN_SD_SCK   40
#define PIN_SD_MISO  39
#define PIN_SD_MOSI  14
#define PIN_SD_CS    12

#define TFT_FREQ     27000000  // если картинка рябит: 20000000
#define SCREEN_W 320
#define SCREEN_H 240

#define RGB565(r,g,b) ((uint16_t)((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3)))

// Настройки интерфейса (меняются в приложении "Настройки", хранятся в NVS)
struct UiConfig {
  uint8_t theme;      // номер темы
  uint8_t accent;     // номер акцентного цвета
  uint8_t rotation;   // 1, 3, 5 или 7
  uint8_t invert;     // инверсия цветов экрана
  uint16_t bg, fg, dim, accent_c, sel;  // текущие цвета (считаются из темы)
};
extern UiConfig g_cfg;

#define C_BG     (g_cfg.bg)
#define C_FG     (g_cfg.fg)
#define C_DIM    (g_cfg.dim)
#define C_ACCENT (g_cfg.accent_c)
#define C_SEL    (g_cfg.sel)
#define C_OK     RGB565(0,200,80)
#define C_ERR    RGB565(230,50,50)
