#pragma once
#include <SD.h>
#include <SPI.h>
#include "config.h"

extern bool g_sdOk;

inline bool storageBegin() {
  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  g_sdOk = SD.begin(PIN_SD_CS, SPI, 20000000);
  return g_sdOk;
}