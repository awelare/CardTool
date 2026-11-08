#pragma once
// Чтение файлов Flipper Zero (.ir) с SD и отправка сигналов
#include <SD.h>
#include <string.h>
#include <stdlib.h>
#include "ir.h"

enum IrKind : uint8_t { IK_NONE = 0, IK_NEC, IK_NECEXT, IK_SAMSUNG, IK_RAW };

struct IrButton {
  char name[28];
  uint8_t kind;
  uint16_t n;          // raw: число значений
  uint32_t addr, cmd, freq;
  uint32_t pos;        // raw: позиция строки "data:" в файле
  const uint16_t* rawp; // raw из прошивки (встроенные профили), иначе nullptr
};

static const int IR_MAX_BTN = 48;
static const int IR_MAX_RAW = 1200;

inline const char* irKindName(uint8_t k) {
  switch (k) {
    case IK_NEC: return "NEC";
    case IK_NECEXT: return "NECext";
    case IK_SAMSUNG: return "Samsung";
    case IK_RAW: return "RAW";
    default: return "-";
  }
}

inline uint32_t irParseHex4(const char* s) {
  unsigned b[4] = {0, 0, 0, 0};
  sscanf(s, "%x %x %x %x", &b[0], &b[1], &b[2], &b[3]);
  return b[0] | (b[1] << 8) | (b[2] << 16) | ((uint32_t)b[3] << 24);
}

inline int irCountTokens(const char* s) {
  int n = 0; bool in = false;
  for (; *s; s++) {
    if (*s >= '0' && *s <= '9') { if (!in) { n++; in = true; } }
    else in = false;
  }
  return n;
}

// Возвращает число кнопок или -1 при ошибке открытия
inline int irLoadFile(const char* path, IrButton* out, int maxN) {
  File f = SD.open(path, FILE_READ);
  if (!f) return -1;
  int n = 0;
  IrButton cur;
  memset(&cur, 0, sizeof(cur));
  bool have = false, isRaw = false;
  char proto[16] = "";

  auto commit = [&]() {
    if (!have || n >= maxN) return;
    if (isRaw && cur.n > 0) cur.kind = IK_RAW;
    else if (!strcmp(proto, "NEC")) cur.kind = IK_NEC;
    else if (!strcmp(proto, "NECext")) cur.kind = IK_NECEXT;
    else if (!strcmp(proto, "Samsung32")) cur.kind = IK_SAMSUNG;
    else cur.kind = IK_NONE;
    out[n++] = cur;
    have = false;
  };

  while (f.available()) {
    uint32_t lp = f.position();
    String ln = f.readStringUntil('\n');
    ln.trim();
    if (ln.length() == 0 || ln[0] == '#') continue;
    int c = ln.indexOf(':');
    if (c < 0) continue;
    String key = ln.substring(0, c);
    String val = ln.substring(c + 1);
    val.trim();
    if (key == "name") {
      commit();
      memset(&cur, 0, sizeof(cur));
      strncpy(cur.name, val.c_str(), sizeof(cur.name) - 1);
      cur.freq = 38000;
      isRaw = false;
      proto[0] = 0;
      have = true;
    } else if (!have) {
      continue;
    } else if (key == "type") {
      isRaw = (val == "raw");
    } else if (key == "protocol") {
      strncpy(proto, val.c_str(), sizeof(proto) - 1);
      proto[sizeof(proto) - 1] = 0;
    } else if (key == "address") {
      cur.addr = irParseHex4(val.c_str());
    } else if (key == "command") {
      cur.cmd = irParseHex4(val.c_str());
    } else if (key == "frequency") {
      cur.freq = strtoul(val.c_str(), nullptr, 10);
    } else if (key == "data") {
      cur.pos = lp;
      cur.n = irCountTokens(val.c_str());
    }
  }
  commit();
  f.close();
  return n;
}

inline bool irSendButton(const char* path, const IrButton& b) {
  uint16_t d[70];
  switch (b.kind) {
    case IK_NEC: {
      uint8_t a = b.addr & 0xFF, c = b.cmd & 0xFF;
      uint32_t x = a | ((uint32_t)(uint8_t)~a << 8) | ((uint32_t)c << 16) | ((uint32_t)(uint8_t)~c << 24);
      ir::play(d, ir::buildNEC(d, x, false), 38000);
      return true;
    }
    case IK_NECEXT: {
      uint32_t x = (b.addr & 0xFFFF) | ((b.cmd & 0xFFFF) << 16);
      ir::play(d, ir::buildNEC(d, x, false), 38000);
      return true;
    }
    case IK_SAMSUNG: {
      uint8_t a = b.addr & 0xFF, c = b.cmd & 0xFF;
      uint32_t x = a | ((uint32_t)a << 8) | ((uint32_t)c << 16) | ((uint32_t)(uint8_t)~c << 24);
      ir::play(d, ir::buildNEC(d, x, true), 38000);
      return true;
    }
    case IK_RAW: {
      if (b.rawp && b.n > 0) { ir::play(b.rawp, b.n, b.freq ? b.freq : 38000); return true; }
      if (b.n == 0 || b.n > IR_MAX_RAW) return false;
      File f = SD.open(path, FILE_READ);
      if (!f) return false;
      if (!f.seek(b.pos)) { f.close(); return false; }
      String ln = f.readStringUntil('\n');
      f.close();
      int c = ln.indexOf(':');
      if (c < 0) return false;
      uint16_t* buf = (uint16_t*)malloc(b.n * sizeof(uint16_t));
      if (!buf) return false;
      const char* s = ln.c_str() + c + 1;
      int k = 0;
      while (*s && k < b.n) {
        while (*s && (*s < '0' || *s > '9')) s++;
        if (!*s) break;
        char* e;
        unsigned long v = strtoul(s, &e, 10);
        s = e;
        buf[k++] = v > 65535 ? 65535 : (uint16_t)v;
      }
      if (k > 0) ir::play(buf, k, b.freq);
      free(buf);
      return k > 0;
    }
    default:
      return false;
  }
}
