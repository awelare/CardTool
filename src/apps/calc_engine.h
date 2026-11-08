#pragma once
// Движок инженерного калькулятора: разбор выражений (без привязки к экрану)
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

namespace calc {

struct Ctx {
  bool deg = true;    // углы в градусах
  double ans = 0;     // последний результат
};

static const double PI_ = 3.14159265358979323846;

class Parser {
public:
  const char* err = "";
  Parser(const char* s, const Ctx& c) : p(s), ctx(c) {}

  bool run(double& out) {
    skip();
    if (!*p) { err = "Пусто"; return false; }
    double v = expr();
    if (!ok) return false;
    skip();
    if (*p) { err = "Лишний символ"; return false; }
    out = v;
    return true;
  }

private:
  const char* p;
  const Ctx& ctx;
  bool ok = true;

  void skip() { while (*p == ' ') p++; }
  void fail(const char* e) { if (ok) { ok = false; err = e; } }

  double expr() {
    double v = term();
    while (ok) {
      skip();
      if (*p == '+') { p++; v += term(); }
      else if (*p == '-') { p++; v -= term(); }
      else break;
    }
    return v;
  }

  double term() {
    double v = unary();
    while (ok) {
      skip();
      if (*p == '*') { p++; v *= unary(); }
      else if (*p == '/') {
        p++;
        double d = unary();
        if (ok && d == 0) { fail("Деление на 0"); return 0; }
        v /= d;
      } else if (*p == '%') {
        p++;
        double d = unary();
        if (ok && d == 0) { fail("Деление на 0"); return 0; }
        v = fmod(v, d);
      } else if (*p == '(' || *p == '.' || *p == '_' || isalnum((unsigned char)*p)) {
        v *= power();   // неявное умножение: 2pi, 3(4+5)
      } else break;
    }
    return v;
  }

  double unary() {
    skip();
    if (*p == '-') { p++; return -unary(); }
    if (*p == '+') { p++; return unary(); }
    return power();
  }

  double power() {
    double b = postfix();
    skip();
    if (ok && *p == '^') {
      p++;
      double e = unary();
      return pow(b, e);
    }
    return b;
  }

  double postfix() {
    double v = primary();
    while (ok) {
      skip();
      if (*p == '!') {
        p++;
        if (v < 0 || v > 170 || v != floor(v)) { fail("n! только целые 0..170"); return 0; }
        v = round(tgamma(v + 1));
      } else break;
    }
    return v;
  }

  double primary() {
    skip();
    if (!ok) return 0;
    if (*p == '(') {
      p++;
      double v = expr();
      skip();
      if (ok && *p != ')') { fail("Нет закрывающей )"); return 0; }
      if (ok) p++;
      return v;
    }
    if (isdigit((unsigned char)*p) || *p == '.') {
      char* e;
      double v = strtod(p, &e);
      if (e == p) { fail("Неверное число"); return 0; }
      p = e;
      return v;
    }
    if (isalpha((unsigned char)*p) || *p == '_') {
      char name[12];
      int n = 0;
      while ((isalnum((unsigned char)*p) || *p == '_') && n < 11) name[n++] = (char)tolower((unsigned char)*p++);
      name[n] = 0;
      if (isalnum((unsigned char)*p) || *p == '_') { fail("Неизвестное имя"); return 0; }
      skip();
      if (*p == '(') {
        p++;
        double a = expr();
        skip();
        if (ok && *p != ')') { fail("Нет закрывающей )"); return 0; }
        if (!ok) return 0;
        p++;
        return func(name, a);
      }
      return constant(name);
    }
    fail("Ожидалось число");
    return 0;
  }

  static bool isFunc(const char* n) {
    static const char* f[] = {"sin","cos","tan","asin","acos","atan","sinh","cosh","tanh",
                              "ln","log","lg","log2","sqrt","cbrt","abs","exp","floor","ceil","round"};
    for (unsigned i = 0; i < sizeof(f) / sizeof(f[0]); i++) if (!strcmp(n, f[i])) return true;
    return false;
  }

  double constant(const char* n) {
    if (!strcmp(n, "pi")) return PI_;
    if (!strcmp(n, "e")) return 2.71828182845904523536;
    if (!strcmp(n, "tau")) return 2 * PI_;
    if (!strcmp(n, "phi")) return 1.61803398874989484820;
    if (!strcmp(n, "ans")) return ctx.ans;
    if (isFunc(n)) { fail("Нужны скобки"); return 0; }
    fail("Неизвестное имя");
    return 0;
  }

  double toRad(double x) const { return ctx.deg ? x * PI_ / 180.0 : x; }
  double fromRad(double x) const { return ctx.deg ? x * 180.0 / PI_ : x; }
  static double clean(double x) { return fabs(x) < 1e-14 ? 0.0 : x; }

  double func(const char* n, double a) {
    if (!strcmp(n, "sin")) return clean(sin(toRad(a)));
    if (!strcmp(n, "cos")) return clean(cos(toRad(a)));
    if (!strcmp(n, "tan")) {
      double c = cos(toRad(a));
      if (fabs(c) < 1e-14) { fail("tan не определён"); return 0; }
      return clean(sin(toRad(a)) / c);
    }
    if (!strcmp(n, "asin")) { if (fabs(a) > 1) { fail("Аргумент вне [-1;1]"); return 0; } return fromRad(asin(a)); }
    if (!strcmp(n, "acos")) { if (fabs(a) > 1) { fail("Аргумент вне [-1;1]"); return 0; } return fromRad(acos(a)); }
    if (!strcmp(n, "atan")) return fromRad(atan(a));
    if (!strcmp(n, "sinh")) return sinh(a);
    if (!strcmp(n, "cosh")) return cosh(a);
    if (!strcmp(n, "tanh")) return tanh(a);
    if (!strcmp(n, "ln")) { if (a <= 0) { fail("ln: аргумент <= 0"); return 0; } return log(a); }
    if (!strcmp(n, "log") || !strcmp(n, "lg")) { if (a <= 0) { fail("log: аргумент <= 0"); return 0; } return log10(a); }
    if (!strcmp(n, "log2")) { if (a <= 0) { fail("log2: аргумент <= 0"); return 0; } return log2(a); }
    if (!strcmp(n, "sqrt")) { if (a < 0) { fail("Корень из отрицательного"); return 0; } return sqrt(a); }
    if (!strcmp(n, "cbrt")) return cbrt(a);
    if (!strcmp(n, "abs")) return fabs(a);
    if (!strcmp(n, "exp")) return exp(a);
    if (!strcmp(n, "floor")) return floor(a);
    if (!strcmp(n, "ceil")) return ceil(a);
    if (!strcmp(n, "round")) return round(a);
    fail("Неизвестная функция");
    return 0;
  }
};

inline bool eval(const char* s, const Ctx& c, double& out, const char** err) {
  Parser ps(s, c);
  if (!ps.run(out)) { *err = ps.err; return false; }
  if (!isfinite(out)) { *err = "Переполнение"; return false; }
  return true;
}

inline void fmt(double v, char* out, size_t n) {
  if (v == 0) v = 0;   // убрать -0
  snprintf(out, n, "%.10g", v);
}

}  // namespace calc
