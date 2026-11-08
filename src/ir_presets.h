#pragma once
// Встроенные профили ИК-пульта (работают без SD-карты)
#include <stdint.h>
#include "ir_file.h"

struct IrPreset {
  const char* name;
  uint8_t kind;          // IK_NEC, IK_RAW ...
  uint32_t addr, cmd;    // для NEC
  const uint16_t* raw;   // для RAW: значения в мкс (импульс, пауза, ...)
  uint16_t n;
};

struct IrDevice {
  const char* name;
  const IrPreset* buttons;
  uint8_t count;
};

// ---------- Mostech (NEC, адрес 0x08) ----------
static const uint16_t kMosIdkRaw[] = {174, 2511, 76, 5985, 318};
static const uint16_t kMosMuteRaw[] = {
  9003, 4353, 647, 461, 648, 463, 646, 462, 648, 1572, 646, 463, 647, 463, 646, 462,
  647, 460, 649, 1572, 647, 1571, 647, 1571, 648, 462, 647, 1572, 647, 1571, 648, 1572,
  647, 1572, 647, 462, 647, 1570, 648, 1572, 647, 1570, 648, 460, 649, 1573, 645, 463,
  646, 1571, 648, 1571, 647, 461, 649, 461, 648, 462, 647, 1571, 648, 463, 646, 1572,
  647, 461, 648, 40021, 8952, 2186, 648};

static const IrPreset kMostech[] = {
  {"idk",                    IK_RAW, 0, 0, kMosIdkRaw, sizeof(kMosIdkRaw) / 2},
  {"Mos_power.off",          IK_NEC, 0x08, 0xD7, nullptr, 0},
  {"Mos_up",                 IK_NEC, 0x08, 0x92, nullptr, 0},
  {"Mos_left_presentation",  IK_NEC, 0x08, 0x97, nullptr, 0},
  {"Mos_right_presentation", IK_NEC, 0x08, 0x9F, nullptr, 0},
  {"Mos_down",               IK_NEC, 0x08, 0xD8, nullptr, 0},
  {"Mos_ok",                 IK_NEC, 0x08, 0x9B, nullptr, 0},
  {"Mos_vol++",              IK_NEC, 0x08, 0x83, nullptr, 0},
  {"Mos_vol--",              IK_NEC, 0x08, 0x86, nullptr, 0},
  {"Mos_mute",               IK_RAW, 0, 0, kMosMuteRaw, sizeof(kMosMuteRaw) / 2},
  {"Mos_settings",           IK_NEC, 0x08, 0xB4, nullptr, 0},
  {"Mos_android",            IK_NEC, 0x08, 0xAD, nullptr, 0},
  {"Mos_menu",               IK_NEC, 0x08, 0xD4, nullptr, 0},
};

// ---------- Irbis (NEC, адрес 0x01) ----------
static const IrPreset kIrbis[] = {
  {"POWER",    IK_NEC, 0x01, 0x18, nullptr, 0},
  {"VOL+",     IK_NEC, 0x01, 0x14, nullptr, 0},
  {"VOL-",     IK_NEC, 0x01, 0x10, nullptr, 0},
  {"UP",       IK_NEC, 0x01, 0x46, nullptr, 0},
  {"LEFT",     IK_NEC, 0x01, 0x47, nullptr, 0},
  {"RIGHT",    IK_NEC, 0x01, 0x15, nullptr, 0},
  {"DOWN",     IK_NEC, 0x01, 0x16, nullptr, 0},
  {"MENU",     IK_NEC, 0x01, 0x40, nullptr, 0},
  {"SOURCE",   IK_NEC, 0x01, 0x0D, nullptr, 0},
  {"BLANK",    IK_NEC, 0x01, 0x04, nullptr, 0},
  {"FREEZE",   IK_NEC, 0x01, 0x12, nullptr, 0},
  {"OK",       IK_NEC, 0x01, 0x55, nullptr, 0},
  {"SETTINGS", IK_NEC, 0x01, 0x25, nullptr, 0},
};

static const IrDevice kIrDevices[] = {
  {"Mostech", kMostech, sizeof(kMostech) / sizeof(kMostech[0])},
  {"Irbis",   kIrbis,   sizeof(kIrbis) / sizeof(kIrbis[0])},
};
static const int kIrDeviceCount = sizeof(kIrDevices) / sizeof(kIrDevices[0]);
