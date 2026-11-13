#pragma once
// Вывод формул и текста с разметкой:
//   a_1  a_{12}   нижний индекс        x^2  x^{n+1}   верхний индекс
//   \frac{числитель}{знаменатель}      \sqrt{под корнем}
//   \alpha \Delta \pi \cdot \to ...    греческие буквы и знаки (список в kSyms)
//   \_ \^ \\ \{    сами символы _ ^ \ {
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "../ui.h"

#ifndef UI_FONT_SMALL
#define UI_FONT_SMALL (&fonts::efontJA_10)
#endif

namespace rich {

struct M { int w, asc, desc; };   // ширина, высота над базовой линией, под ней

inline uint16_t g_col = 0xFFFF;
inline void setColor(uint16_t c) { g_col = c; }

static const int ASC_N = 13, DESC_N = 3;   // основной шрифт (16 px)
static const int ASC_S = 8,  DESC_S = 2;   // мелкий шрифт для индексов (10 px)

struct Sym { const char* name; const char* utf8; };
static const Sym kSyms[] = {
  {"alpha","α"},{"beta","β"},{"gamma","γ"},{"delta","δ"},{"epsilon","ε"},{"eta","η"},{"theta","θ"},
  {"lambda","λ"},{"mu","μ"},{"nu","ν"},{"pi","π"},{"rho","ρ"},{"sigma","σ"},{"tau","τ"},{"phi","φ"},
  {"omega","ω"},{"Delta","Δ"},{"Gamma","Γ"},{"Theta","Θ"},{"Lambda","Λ"},{"Sigma","Σ"},{"Phi","Φ"},
  {"Omega","Ω"},{"Pi","Π"},{"cdot","·"},{"times","×"},{"pm","±"},{"neq","≠"},{"leq","≤"},{"geq","≥"},
  {"approx","≈"},{"to","→"},{"deg","°"},{"infty","∞"},{"in","∈"},{"angle","∠"},{"perp","⊥"},
  {"sin","sin"},{"cos","cos"},{"tg","tg"},{"ctg","ctg"},{"lg","lg"},{"ln","ln"},{"log","log"},
};

// Читает аргумент: {группа} или один символ UTF-8. Возвращает указатель после аргумента.
inline const char* readArg(const char* s, char* out, int cap) {
  int n = 0;
  if (*s == '{') {
    int depth = 1;
    s++;
    while (*s) {
      if (*s == '{') depth++;
      else if (*s == '}') { depth--; if (depth == 0) { s++; break; } }
      if (n < cap - 1) out[n++] = *s;
      s++;
    }
  } else if (*s) {
    unsigned char c = (unsigned char)*s;
    int len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
    for (int k = 0; k < len && *s; k++) { if (n < cap - 1) out[n++] = *s; s++; }
  }
  out[n] = 0;
  return s;
}

// Рисует (draw=true) или только измеряет строку. base — базовая линия, x0 — левый край.
inline M render(const char* s, int x0, int base, bool draw, bool small = false) {
  M m{0, small ? ASC_S : ASC_N, small ? DESC_S : DESC_N};
  const int asc = m.asc;
  int x = x0;
  char run[160];
  int rn = 0;
  auto flush = [&]() {
    if (!rn) return;
    run[rn] = 0;
    gfx.setFont(small ? UI_FONT_SMALL : UI_FONT);
    int w = gfx.textWidth(run);
    if (draw) { gfx.setTextColor(g_col); gfx.drawString(run, x, base - asc); }
    x += w;
    rn = 0;
  };
  auto addc = [&](char c) {
    if (rn > 150 && (((unsigned char)c) & 0xC0) != 0x80) flush();
    if (rn < 158) run[rn++] = c;
  };

  while (*s) {
    char c = *s;
    if (c == '_' || c == '^') {
      flush();
      s++;
      char arg[80];
      s = readArg(s, arg, sizeof(arg));
      int dy = (c == '_') ? (small ? 3 : 4) : (small ? -5 : -6);
      M sm = render(arg, x, base + dy, draw, true);
      x += sm.w;
      if (c == '_') { int d = dy + sm.desc; if (d > m.desc) m.desc = d; }
      else          { int a = -dy + sm.asc; if (a > m.asc) m.asc = a; }
    } else if (c == '\\') {
      s++;
      char cmd[12];
      int cn = 0;
      while (isalpha((unsigned char)*s) && cn < 11) cmd[cn++] = *s++;
      cmd[cn] = 0;
      if (cn == 0) {                         // экранированный символ
        if (*s) { addc(*s); s++; }
        continue;
      }
      if (!strcmp(cmd, "frac")) {
        char A[80], B[80];
        s = readArg(s, A, sizeof(A));
        s = readArg(s, B, sizeof(B));
        flush();
        M ma = render(A, 0, 0, false, small), mb = render(B, 0, 0, false, small);
        int W = (ma.w > mb.w ? ma.w : mb.w) + 6;
        int axis = base - (small ? 3 : 5);
        int nb = axis - 3 - ma.desc;
        int db = axis + 3 + mb.asc;
        if (draw) {
          render(A, x + (W - ma.w) / 2, nb, true, small);
          render(B, x + (W - mb.w) / 2, db, true, small);
          gfx.drawFastHLine(x + 1, axis, W - 2, g_col);
        }
        int a = (base - nb) + ma.asc, d = (db - base) + mb.desc;
        if (a > m.asc) m.asc = a;
        if (d > m.desc) m.desc = d;
        x += W;
      } else if (!strcmp(cmd, "sqrt")) {
        char A[80];
        s = readArg(s, A, sizeof(A));
        flush();
        M ma = render(A, 0, 0, false, small);
        int top = base - ma.asc - 2;
        if (draw) {
          gfx.drawLine(x, base - 5, x + 2, base - 3, g_col);
          gfx.drawLine(x + 2, base - 3, x + 5, base + 2, g_col);
          gfx.drawLine(x + 5, base + 2, x + 9, top, g_col);
          gfx.drawFastHLine(x + 9, top, ma.w + 3, g_col);
          render(A, x + 11, base, true, small);
        }
        if (ma.asc + 3 > m.asc) m.asc = ma.asc + 3;
        if (ma.desc > m.desc) m.desc = ma.desc;
        x += 11 + ma.w + 2;
      } else {
        bool found = false;
        for (unsigned i = 0; i < sizeof(kSyms) / sizeof(kSyms[0]); i++)
          if (!strcmp(cmd, kSyms[i].name)) { for (const char* u = kSyms[i].utf8; *u; u++) addc(*u); found = true; break; }
        if (!found) { addc('\\'); for (const char* u = cmd; *u; u++) addc(*u); }
      }
    } else {
      addc(c);
      s++;
    }
  }
  flush();
  m.w = x - x0;
  return m;
}

// Абзац с переносом по словам (пробелы внутри {} не разрывают слово). Возвращает высоту в пикселях.
inline int paragraph(const char* text, int x, int y, int maxW, bool draw, int lineH = 20) {
  gfx.setFont(UI_FONT);
  int space = gfx.textWidth(" ");
  int cx = x, line = 0;
  const char* p = text;
  char word[120];
  while (*p) {
    while (*p == ' ') p++;
    if (!*p) break;
    int n = 0, depth = 0;
    while (*p && (*p != ' ' || depth > 0)) {
      if (*p == '{') depth++;
      if (*p == '}' && depth > 0) depth--;
      if (n < (int)sizeof(word) - 1) word[n++] = *p;
      p++;
    }
    word[n] = 0;
    M m = render(word, 0, 0, false);
    if (cx > x && cx + m.w > x + maxW) { cx = x; line++; }
    if (draw) render(word, cx, y + line * lineH + 14, true);
    cx += m.w + space;
  }
  return (line + 1) * lineH;
}

}  // namespace rich
