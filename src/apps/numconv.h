#pragma once
// Перевод чисел между системами счисления (целая и дробная часть)
#include <stdint.h>
#include <string.h>

namespace numconv {

inline int digitVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
  return 99;
}
inline char digitChar(int d) { return d < 10 ? (char)('0' + d) : (char)('A' + d - 10); }

struct Num {
  bool neg = false;
  uint64_t ip = 0;     // целая часть
  double fp = 0;       // дробная часть [0;1)
  bool hasFrac = false;
};

inline bool parse(const char* s, int base, Num& n, const char** err) {
  n = Num();
  if (!s || !*s) { *err = "Пусто"; return false; }
  if (*s == '-') { n.neg = true; s++; }
  else if (*s == '+') s++;
  if (!*s) { *err = "Пусто"; return false; }
  bool any = false;
  for (; *s && *s != '.'; s++) {
    int d = digitVal(*s);
    if (d >= base) { *err = "Цифра вне системы"; return false; }
    if (n.ip > (UINT64_MAX - (uint64_t)d) / (uint64_t)base) { *err = "Слишком большое число"; return false; }
    n.ip = n.ip * (uint64_t)base + (uint64_t)d;
    any = true;
  }
  if (*s == '.') {
    s++;
    n.hasFrac = true;
    double scale = 1.0 / base;
    for (int k = 0; *s; s++, k++) {
      int d = digitVal(*s);
      if (d >= base) { *err = "Цифра вне системы"; return false; }
      if (k < 40) { n.fp += d * scale; scale /= base; }
      any = true;
    }
  }
  if (!any) { *err = "Нет цифр"; return false; }
  return true;
}

// Запись числа в системе base. cap >= 100 рекомендуется.
inline void toBase(const Num& n, int base, char* out, int cap, int maxFrac = 12) {
  char tmp[72];
  int t = 0;
  uint64_t v = n.ip;
  if (v == 0) tmp[t++] = '0';
  while (v > 0 && t < 70) { tmp[t++] = digitChar((int)(v % (uint64_t)base)); v /= (uint64_t)base; }
  int o = 0;
  bool zero = (n.ip == 0 && n.fp < 1e-12);
  if (n.neg && !zero && o < cap - 1) out[o++] = '-';
  while (t > 0 && o < cap - 1) out[o++] = tmp[--t];
  if (n.hasFrac && n.fp > 1e-12 && o < cap - 2) {
    out[o++] = '.';
    double f = n.fp;
    int k = 0;
    while (k < maxFrac && f > 1e-12 && o < cap - 5) {
      f *= base;
      int d = (int)f;
      if (d >= base) d = base - 1;
      out[o++] = digitChar(d);
      f -= d;
      k++;
    }
    if (f > 1e-9 && o < cap - 4) { out[o++] = '.'; out[o++] = '.'; out[o++] = '.'; }
  }
  out[o] = 0;
}

}  // namespace numconv
