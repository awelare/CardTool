#pragma once
#include <M5GFX.h>
#include <lgfx/v1/panel/Panel_LCD.hpp>
#include "config.h"


// В M5GFX нет панели ILI9341, описываем сами (на основе Panel_LCD)
struct Panel_ILI9341_Local : public lgfx::Panel_LCD {
  Panel_ILI9341_Local() {
    _cfg.memory_width  = _cfg.panel_width  = 240;
    _cfg.memory_height = _cfg.panel_height = 320;
  }
protected:
  const uint8_t* getInitCommands(uint8_t listno) const override {
    static constexpr uint8_t list0[] = {
      0xEF, 3, 0x03, 0x80, 0x02,
      0xCF, 3, 0x00, 0xC1, 0x30,
      0xED, 4, 0x64, 0x03, 0x12, 0x81,
      0xE8, 3, 0x85, 0x00, 0x78,
      0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
      0xF7, 1, 0x20,
      0xEA, 2, 0x00, 0x00,
      0xC0, 1, 0x23,
      0xC1, 1, 0x10,
      0xC5, 2, 0x3E, 0x28,
      0xC7, 1, 0x86,
      0xB1, 2, 0x00, 0x13,
      0xB6, 3, 0x08, 0x82, 0x27,
      0xF2, 1, 0x00,
      0x26, 1, 0x01,
      0xE0, 15, 0x0F,0x31,0x2B,0x0C,0x0E,0x08,0x4E,0xF1,0x37,0x07,0x10,0x03,0x0E,0x09,0x00,
      0xE1, 15, 0x00,0x0E,0x14,0x03,0x11,0x07,0x31,0xC1,0x48,0x08,0x0F,0x0C,0x31,0x36,0x0F,
      0xB7, 1, 0x07,
      0x11, 0 + CMD_INIT_DELAY, 120,   // sleep out
      0x38, 0,                         // idle mode off
      0x29, 0 + CMD_INIT_DELAY, 100,   // display on
      0xFF, 0xFF,
    };
    switch (listno) {
      case 0: return list0;
      default: return nullptr;
    }
  }
};

class LGFX_ILI9341 : public lgfx::LGFX_Device {
  Panel_ILI9341_Local _panel;
  lgfx::Bus_SPI _bus;
public:
  LGFX_ILI9341() {
    {
      auto c = _bus.config();
      c.spi_host = SPI2_HOST;
      c.spi_mode = 0;
      c.freq_write = TFT_FREQ;
      c.freq_read = 16000000;
      c.spi_3wire = false;
      c.use_lock = true;      // шина общая с SD
      c.dma_channel = SPI_DMA_CH_AUTO;
      c.pin_sclk = PIN_TFT_SCK;
      c.pin_mosi = PIN_TFT_MOSI;
      c.pin_miso = PIN_SD_MISO;   // 39 — SD-карта отдаёт данные по этой линии
      c.pin_dc = PIN_TFT_DC;
      _bus.config(c);
      _panel.setBus(&_bus);
    }
    {
      auto c = _panel.config();
      c.pin_cs = PIN_TFT_CS;
      c.pin_rst = PIN_TFT_RST;
      c.pin_busy = -1;
      c.panel_width = 240;
      c.panel_height = 320;
      c.offset_x = 0;
      c.offset_y = 0;
      c.offset_rotation = 0;
      c.readable = false;
      c.invert = false;       // если цвета инвертированы: true
      c.rgb_order = false;    // если красный/синий перепутаны: true
      c.dlen_16bit = false;
      c.bus_shared = true;
      _panel.config(c);
    }
    setPanel(&_panel);
  }
};

extern LGFX_ILI9341 gfx;

// Шрифт с кириллицей (проверяется в приложении "Тест железа")
#define UI_FONT (&fonts::efontJA_16)
