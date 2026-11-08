#pragma once
// Целочисленная математика в разных системах счисления (64 бита)
// Числа: по умолчанию в выбранной системе, либо с явной системой: 101#2, FF#16, 17#8
// Операции: + - * / % & | ^ ~ << >> и слова: and or xor not shl shr mod
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "numconv.h"

namespace prog {

enum : uint8_t { T_NUM, T_OP, T_LP, T_RP };
struct Tok { uint8_t t; int64_t v; };
static const int MAXTOK = 64;

class Eval {
public:
  const char* err = "";

  bool run(const char* s, int base, int64_t& out) {
    if (!tokenize(s, base)) return false;
    if (_n == 0) { err = "Пусто"; return false; }
    _i = 0;
    int64_t v = orExpr();
    if (!_ok) return false;
    if (_i != _n) { err = "Лишнее в конце"; return false; }
    out = v;
    return true;
  }

private:
  Tok _t[MAXTOK];
  int _n = 0, _i = 0;
  bool _ok = true;

  void fail(const char* e) { if (_ok) { _ok = false; err = e; } }

  static bool word(const char* a, int la, const char* w) {
    return (int)strlen(w) == la && strncasecmp(a, w, la) == 0;
  }

  bool push(uint8_t t, int64_t v) {
    if (_n >= MAXTOK) { err = "Слишком длинно"; return false; }
    _t[_n].t = t; _t[_n].v = v; _n++;
    return true;
  }

  bool tokenize(const char* s, int base) {
    _n = 0;
    while (*s) {
      char c = *s;
      if (c == ' ') { s++; continue; }
      if (isalnum((unsigned char)c) || c == '#') {
        const char* st = s;
        while (isalnum((unsigned char)*s) || *s == '#') s++;
        int len = (int)(s - st);
        if (word(st, len, "and")) { if (!push(T_OP, '&')) return false; continue; }
        if (word(st, len, "or"))  { if (!push(T_OP, '|')) return false; continue; }
        if (word(st, len, "xor")) { if (!push(T_OP, '^')) return false; continue; }
        if (word(st, len, "not")) { if (!push(T_OP, '~')) return false; continue; }
        if (word(st, len, "shl")) { if (!push(T_OP, 'L')) return false; continue; }
        if (word(st, len, "shr")) { if (!push(T_OP, 'R')) return false; continue; }
        if (word(st, len, "mod")) { if (!push(T_OP, '%')) return false; continue; }
        int b = base, dlen = len;
        const char* hash = (const char*)memchr(st, '#', len);
        if (hash) {
          dlen = (int)(hash - st);
          int bl = len - dlen - 1;
          if (bl <= 0 || bl > 2) { err = "Система после # (2..36)"; return false; }
          b = 0;
          for (int k = 0; k < bl; k++) {
            char d = hash[1 + k];
            if (d < '0' || d > '9') { err = "Система после # (2..36)"; return false; }
            b = b * 10 + (d - '0');
          }
          if (b < 2 || b > 36) { err = "Система после # (2..36)"; return false; }
        }
        if (dlen <= 0) { err = "Нет цифр перед #"; return false; }
        uint64_t v = 0;
        for (int k = 0; k < dlen; k++) {
          int d = numconv::digitVal(st[k]);
          if (d >= b) { err = "Цифра вне системы"; return false; }
          if (v > (UINT64_MAX - (uint64_t)d) / (uint64_t)b) { err = "Слишком большое число"; return false; }
          v = v * (uint64_t)b + (uint64_t)d;
        }
        if (!push(T_NUM, (int64_t)v)) return false;
        continue;
      }
      switch (c) {
        case '+': case '-': case '*': case '/': case '%': case '&': case '|': case '^': case '~':
          if (!push(T_OP, c)) return false; s++; break;
        case '(': if (!push(T_LP, 0)) return false; s++; break;
        case ')': if (!push(T_RP, 0)) return false; s++; break;
        case '<':
          if (s[1] != '<') { err = "Нужно <<"; return false; }
          if (!push(T_OP, 'L')) return false; s += 2; break;
        case '>':
          if (s[1] != '>') { err = "Нужно >>"; return false; }
          if (!push(T_OP, 'R')) return false; s += 2; break;
        default:
          err = "Неизвестный символ"; return false;
      }
    }
    return true;
  }

  bool isOp(char c) const { return _i < _n && _t[_i].t == T_OP && _t[_i].v == c; }

  int64_t orExpr() {
    int64_t v = xorExpr();
    while (_ok && isOp('|')) { _i++; v = (int64_t)((uint64_t)v | (uint64_t)xorExpr()); }
    return v;
  }
  int64_t xorExpr() {
    int64_t v = andExpr();
    while (_ok && isOp('^')) { _i++; v = (int64_t)((uint64_t)v ^ (uint64_t)andExpr()); }
    return v;
  }
  int64_t andExpr() {
    int64_t v = shiftExpr();
    while (_ok && isOp('&')) { _i++; v = (int64_t)((uint64_t)v & (uint64_t)shiftExpr()); }
    return v;
  }
  int64_t shiftExpr() {
    int64_t v = addExpr();
    while (_ok && (isOp('L') || isOp('R'))) {
      bool left = isOp('L');
      _i++;
      int64_t k = addExpr();
      if (!_ok) return 0;
      if (k < 0 || k > 63) { fail("Сдвиг 0..63"); return 0; }
      v = left ? (int64_t)((uint64_t)v << k) : (v >> k);
    }
    return v;
  }
  int64_t addExpr() {
    int64_t v = mulExpr();
    while (_ok && (isOp('+') || isOp('-'))) {
      bool plus = isOp('+');
      _i++;
      uint64_t r = (uint64_t)mulExpr();
      v = (int64_t)(plus ? (uint64_t)v + r : (uint64_t)v - r);
    }
    return v;
  }
  int64_t mulExpr() {
    int64_t v = unary();
    while (_ok && (isOp('*') || isOp('/') || isOp('%'))) {
      char op = (char)_t[_i].v;
      _i++;
      int64_t r = unary();
      if (!_ok) return 0;
      if (op == '*') v = (int64_t)((uint64_t)v * (uint64_t)r);
      else {
        if (r == 0) { fail("Деление на 0"); return 0; }
        if (r == -1) v = (op == '/') ? (int64_t)(0 - (uint64_t)v) : 0;
        else v = (op == '/') ? v / r : v % r;
      }
    }
    return v;
  }
  int64_t unary() {
    if (isOp('-')) { _i++; return (int64_t)(0 - (uint64_t)unary()); }
    if (isOp('+')) { _i++; return unary(); }
    if (isOp('~')) { _i++; return (int64_t)(~(uint64_t)unary()); }
    return primary();
  }
  int64_t primary() {
    if (_i >= _n) { fail("Обрыв выражения"); return 0; }
    if (_t[_i].t == T_NUM) return _t[_i++].v;
    if (_t[_i].t == T_LP) {
      _i++;
      int64_t v = orExpr();
      if (!_ok) return 0;
      if (_i >= _n || _t[_i].t != T_RP) { fail("Нет закрывающей )"); return 0; }
      _i++;
      return v;
    }
    fail("Ожидалось число");
    return 0;
  }
};

inline bool eval(const char* s, int base, int64_t& out, const char** err) {
  Eval e;
  if (!e.run(s, base, out)) { *err = e.err; return false; }
  return true;
}

// Запись uint64 в системе base (без знака)
inline void u64ToBase(uint64_t v, int base, char* out, int cap) {
  char tmp[72];
  int t = 0;
  if (v == 0) tmp[t++] = '0';
  while (v > 0 && t < 70) { tmp[t++] = numconv::digitChar((int)(v % (uint64_t)base)); v /= (uint64_t)base; }
  int o = 0;
  while (t > 0 && o < cap - 1) out[o++] = tmp[--t];
  out[o] = 0;
}

}  // namespace prog
