#pragma once
// Вспомогательные функции для ИИ-чата (без привязки к железу, проверяются на ПК)
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

namespace aiu {

// Простой растущий буфер
struct Buf {
  char* p = nullptr;
  size_t n = 0, cap = 0;
  ~Buf() { free(p); }
  void clear() { n = 0; if (p) p[0] = 0; }
  bool reserve(size_t need) {
    if (need + 1 <= cap) return true;
    size_t nc = cap ? cap : 256;
    while (nc < need + 1) nc *= 2;
    char* q = (char*)realloc(p, nc);
    if (!q) return false;
    p = q; cap = nc;
    return true;
  }
  bool add(char c) { if (!reserve(n + 1)) return false; p[n++] = c; p[n] = 0; return true; }
  bool addn(const char* s, size_t k) { if (!reserve(n + k)) return false; memcpy(p + n, s, k); n += k; p[n] = 0; return true; }
  bool add(const char* s) { return addn(s, strlen(s)); }
  const char* c_str() const { return p ? p : ""; }
  size_t size() const { return n; }
};

// ---------- JSON ----------
inline void jsonEscapeAdd(Buf& b, const char* s) {
  for (; *s; s++) {
    unsigned char c = (unsigned char)*s;
    switch (c) {
      case '"':  b.add("\\\""); break;
      case '\\': b.add("\\\\"); break;
      case '\n': b.add("\\n"); break;
      case '\r': b.add("\\r"); break;
      case '\t': b.add("\\t"); break;
      default:
        if (c < 0x20) { char t[8]; snprintf(t, sizeof(t), "\\u%04x", c); b.add(t); }
        else b.add((char)c);
    }
  }
}

inline void addUtf8(Buf& b, uint32_t cp) {
  if (cp < 0x80) b.add((char)cp);
  else if (cp < 0x800) { b.add((char)(0xC0 | (cp >> 6))); b.add((char)(0x80 | (cp & 0x3F))); }
  else if (cp < 0x10000) { b.add((char)(0xE0 | (cp >> 12))); b.add((char)(0x80 | ((cp >> 6) & 0x3F))); b.add((char)(0x80 | (cp & 0x3F))); }
  else { b.add((char)(0xF0 | (cp >> 18))); b.add((char)(0x80 | ((cp >> 12) & 0x3F))); b.add((char)(0x80 | ((cp >> 6) & 0x3F))); b.add((char)(0x80 | (cp & 0x3F))); }
}

inline int hexv(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
inline bool hex4(const char* s, uint32_t& out) {
  out = 0;
  for (int i = 0; i < 4; i++) { int v = s[i] ? hexv(s[i]) : -1; if (v < 0) return false; out = (out << 4) | (uint32_t)v; }
  return true;
}

// Ищет "key": "значение" начиная с from (или с начала), раскодирует строку в out.
inline bool jsonGetString(const char* json, const char* key, const char* from, Buf& out) {
  char pat[48];
  snprintf(pat, sizeof(pat), "\"%s\"", key);
  const char* s = strstr(from ? from : json, pat);
  while (s) {
    const char* q = s + strlen(pat);
    while (*q == ' ' || *q == ':' || *q == '\n' || *q == '\t') q++;
    if (*q == '"') { s = q + 1; break; }
    if (!strncmp(q, "null", 4)) { out.clear(); return false; }
    s = strstr(s + 1, pat);   // это не строка — ищем дальше
  }
  if (!s) return false;
  out.clear();
  while (*s) {
    char c = *s++;
    if (c == '"') return true;
    if (c != '\\') { out.add(c); continue; }
    char e = *s++;
    switch (e) {
      case 'n': out.add('\n'); break;
      case 't': out.add(' '); break;
      case 'r': case 'b': case 'f': break;
      case 'u': {
        uint32_t cp;
        if (!hex4(s, cp)) return true;
        s += 4;
        if (cp >= 0xD800 && cp < 0xDC00 && s[0] == '\\' && s[1] == 'u') {
          uint32_t lo;
          if (hex4(s + 2, lo) && lo >= 0xDC00 && lo < 0xE000) { s += 6; cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00); }
        }
        addUtf8(out, cp);
        break;
      }
      case 0: return true;
      default: out.add(e);   // \" \\ \/
    }
  }
  return false;
}

// убрать Markdown-разметку, которую экран не умеет показывать
inline void stripMarkdown(Buf& b) {
  Buf o;
  const char* s = b.c_str();
  bool lineStart = true;
  while (*s) {
    if (s[0] == '*' && s[1] == '*') { s += 2; continue; }
    if (*s == '`') { s++; continue; }
    if (lineStart && *s == '#') { while (*s == '#') s++; while (*s == ' ') s++; continue; }
    if (lineStart && s[0] == '*' && s[1] == ' ') { o.add("- "); s += 2; lineStart = false; continue; }
    o.add(*s);
    lineStart = (*s == '\n');
    s++;
  }
  b.clear();
  b.add(o.c_str());
}

// ---------- русская раскладка на латинской клавиатуре (ЙЦУКЕН) ----------
// Возвращает UTF-8 строку (до 2 байт + 0) или nullptr, если символ не меняется.
inline const char* ruLower(char c) {
  switch (c) {
    case 'q': return "й"; case 'w': return "ц"; case 'e': return "у"; case 'r': return "к"; case 't': return "е";
    case 'y': return "н"; case 'u': return "г"; case 'i': return "ш"; case 'o': return "щ"; case 'p': return "з";
    case '[': return "х"; case ']': return "ъ"; case 'a': return "ф"; case 's': return "ы"; case 'd': return "в";
    case 'f': return "а"; case 'g': return "п"; case 'h': return "р"; case 'j': return "о"; case 'k': return "л";
    case 'l': return "д"; case ';': return "ж"; case '\'': return "э"; case 'z': return "я"; case 'x': return "ч";
    case 'c': return "с"; case 'v': return "м"; case 'b': return "и"; case 'n': return "т"; case 'm': return "ь";
    case ',': return "б"; case '.': return "ю"; case '/': return "."; case '?': return ","; case '`': return "ё";
    case '&': return "?";
  }
  return nullptr;
}
inline void cyrUpper(char* s) {
  unsigned char a = (unsigned char)s[0], b = (unsigned char)s[1];
  if (a == 0xD0 && b >= 0xB0) s[1] = (char)(b - 0x20);                       // а..п
  else if (a == 0xD1 && b >= 0x80 && b <= 0x8F) { s[0] = (char)0xD0; s[1] = (char)(b + 0x20); }  // р..я
  else if (a == 0xD1 && b == 0x91) { s[0] = (char)0xD0; s[1] = (char)0x81; }                    // ё
}
// Преобразует нажатую клавишу в UTF-8 для русской раскладки. out: минимум 4 байта.
inline void ruKey(char c, char* out) {
  out[0] = c; out[1] = 0;
  if (isupper((unsigned char)c)) {
    const char* m = ruLower((char)tolower((unsigned char)c));
    if (m) { strcpy(out, m); cyrUpper(out); }
    return;
  }
  switch (c) {                      // знаки с Shift
    case '{': strcpy(out, "Х"); return;
    case '}': strcpy(out, "Ъ"); return;
    case ':': strcpy(out, "Ж"); return;
    case '"': strcpy(out, "Э"); return;
    case '<': strcpy(out, "Б"); return;
    case '>': strcpy(out, "Ю"); return;
    case '~': strcpy(out, "Ё"); return;
  }
  const char* m = ruLower(c);
  if (m) strcpy(out, m);
}

// ---------- настройки ----------
struct Cfg {
  char ssid[3][40];
  char pass[3][68];
  int nnet = 0;
  char base[96] = "https://api.groq.com/openai/v1";
  char key[140] = "";
  char model[72] = "llama-3.3-70b-versatile";
  char system[400] = "Ты школьный помощник. Отвечай по-русски, кратко и понятно, 3-6 предложений. Без Markdown и без списков со звёздочками. Формулы пиши простым текстом.";
  int maxTokens = 500;
};

inline char* trimInPlace(char* s) {
  while (*s == ' ' || *s == '\t') s++;
  size_t n = strlen(s);
  while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r')) s[--n] = 0;
  return s;
}
inline void setStr(char* dst, size_t cap, const char* src) { strncpy(dst, src, cap - 1); dst[cap - 1] = 0; }

inline void parseConfig(const char* text, Cfg& c) {
  if ((uint8_t)text[0] == 0xEF && (uint8_t)text[1] == 0xBB && (uint8_t)text[2] == 0xBF) text += 3;
  char line[300];
  while (*text) {
    size_t n = 0;
    while (*text && *text != '\n') { if (n < sizeof(line) - 1) line[n++] = *text; text++; }
    if (*text == '\n') text++;
    line[n] = 0;
    char* l = trimInPlace(line);
    if (!*l || *l == '#') continue;
    char* eq = strchr(l, '=');
    if (!eq) continue;
    *eq = 0;
    char* k = trimInPlace(l);
    char* v = trimInPlace(eq + 1);
    if (!strcasecmp(k, "ssid")) { if (c.nnet < 3) { setStr(c.ssid[c.nnet], sizeof(c.ssid[0]), v); c.pass[c.nnet][0] = 0; c.nnet++; } }
    else if (!strcasecmp(k, "pass")) { if (c.nnet > 0) setStr(c.pass[c.nnet - 1], sizeof(c.pass[0]), v); }
    else if (!strcasecmp(k, "provider")) {
      if (!strcasecmp(v, "groq")) { setStr(c.base, sizeof(c.base), "https://api.groq.com/openai/v1"); }
      else if (!strcasecmp(v, "nvidia")) { setStr(c.base, sizeof(c.base), "https://integrate.api.nvidia.com/v1"); setStr(c.model, sizeof(c.model), "meta/llama-3.3-70b-instruct"); }
    }
    else if (!strcasecmp(k, "base")) setStr(c.base, sizeof(c.base), v);
    else if (!strcasecmp(k, "key")) setStr(c.key, sizeof(c.key), v);
    else if (!strcasecmp(k, "model")) setStr(c.model, sizeof(c.model), v);
    else if (!strcasecmp(k, "system")) setStr(c.system, sizeof(c.system), v);
    else if (!strcasecmp(k, "max_tokens")) { int t = atoi(v); if (t >= 50 && t <= 4000) c.maxTokens = t; }
  }
  size_t bl = strlen(c.base);                       // убрать завершающий /
  while (bl > 0 && c.base[bl - 1] == '/') c.base[--bl] = 0;
}

// Потоковый поиск всех значений "id":"..." в большом JSON (список моделей), без хранения всего ответа
class IdScanner {
public:
  typedef void (*Cb)(void* ctx, const char* id);
  IdScanner(Cb cb, void* ctx) : _cb(cb), _ctx(ctx) {}
  void feed(const uint8_t* d, size_t n) { for (size_t i = 0; i < n; i++) step((char)d[i]); }
private:
  Cb _cb; void* _ctx;
  int _st = 0;            // 0 ищем "id", 1 после "id" ждём ':', 2 ждём '"', 3 читаем значение
  int _m = 0;             // сколько символов "id" совпало
  char _cur[72]; int _cn = 0; bool _esc = false;
  void reset() { _st = 0; _m = 0; }
  void step(char c) {
    switch (_st) {
      case 0: {
        static const char pat[] = "\"id\"";
        if (c == pat[_m]) { _m++; if (_m == 4) _st = 1; }
        else _m = (c == '"') ? 1 : 0;
        break;
      }
      case 1:
        if (c == ':') _st = 2;
        else if (c != ' ' && c != '\n' && c != '\t' && c != '\r') { reset(); if (c == '"') _m = 1; }
        break;
      case 2:
        if (c == '"') { _st = 3; _cn = 0; _esc = false; }
        else if (c != ' ' && c != '\n' && c != '\t' && c != '\r') reset();
        break;
      case 3:
        if (_esc) { if (_cn < (int)sizeof(_cur) - 1) _cur[_cn++] = c; _esc = false; }
        else if (c == '\\') _esc = true;
        else if (c == '"') { _cur[_cn] = 0; if (_cn > 0) _cb(_ctx, _cur); reset(); }
        else if (_cn < (int)sizeof(_cur) - 1) _cur[_cn++] = c;
        break;
    }
  }
};

}  // namespace aiu
