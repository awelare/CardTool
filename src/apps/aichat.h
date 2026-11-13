#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SD.h>
#include <stdio.h>
#include <string.h>
#include "../app.h"
#include "../ui.h"
#include "../storage.h"
#include "ai_util.h"
#include "ai_store.h"

#ifndef AI_CONFIG_PATH
#define AI_CONFIG_PATH "/ai/config.txt"
#endif

// Спросить ИИ: Wi-Fi (точка доступа с телефона) + любой OpenAI-совместимый API.
// Ключи, адреса и модели вводятся на самом устройстве (Enter на пустой строке = настройки).
class AiChatApp : public App {
public:
  void start() override {
    _s.load();
    _scr = CHAT; _in[0] = 0; _inLen = 0; _ru = true;
    _q.clear(); _a.clear(); _status[0] = 0; _scroll = 0; _maxScroll = 0;
    _phase = 0; _wait = 0; _hn = 0; _clear = true; _sel = 0; _first = 0;
  }
  void stop() override { wifiOff(); }

  void onKey(const KeyEvent& e) override {
    if (_phase != 0) return;                           // идёт запрос
    switch (_scr) {
      case CHAT:   onChat(e); break;
      case SET:    onSet(e); break;
      case EDIT:   onEdit(e); break;
      default:     onModels(e); break;
    }
    invalidate();
  }

  void update(uint32_t) override {
    if (_phase == 0) return;
    if (_wait > 0) { _wait--; return; }                // дать экрану перерисоваться
    if (_phase == 1) {                                 // Wi-Fi
      if (connectWifi()) { _phase = _next; _wait = 1; _status[0] = 0; if (_next == 2) snprintf(_status, sizeof(_status), "Думаю..."); else snprintf(_status, sizeof(_status), "Загружаю список моделей..."); }
      else _phase = 0;
      invalidate();
    } else if (_phase == 2) {                          // вопрос
      ask();
      _phase = 0; _scroll = 0;
      invalidate();
    } else if (_phase == 3) {                          // список моделей
      fetchModels();
      _phase = 0;
      invalidate();
    }
  }

  void draw() override {
    gfx.startWrite();
    if (_clear) { gfx.fillScreen(C_BG); _clear = false; }
    switch (_scr) {
      case CHAT:   drawChat(); break;
      case SET:    drawSet(); break;
      case EDIT:   drawEdit(); break;
      default:     drawModels(); break;
    }
    gfx.endWrite();
  }

private:
  enum Scr : uint8_t { CHAT, SET, EDIT, MODELS };
  enum Row : uint8_t { R_PROFILE, R_NAME, R_TEMPLATE, R_BASE, R_KEY, R_MODEL, R_PICK, R_W1, R_P1, R_W2, R_P2, R_IMPORT, R_COUNT };
  static const int HMAX = 3, MAXM = 200;

  AiSettings _s;
  Scr _scr = CHAT;
  bool _ru = true, _ruEdit = false, _clear = true;
  // чат
  char _in[220] = "";
  int _inLen = 0;
  aiu::Buf _q, _a;
  char _status[120] = "";
  int _phase = 0, _wait = 0, _next = 2;      // 0 ждём, 1 Wi-Fi, 2 вопрос, 3 список моделей
  int _scroll = 0, _maxScroll = 0;
  aiu::Buf _hq[HMAX], _ha[HMAX];
  int _hn = 0;
  // настройки / редактор
  int _sel = 0, _first = 0, _editRow = 0, _editCap = 0;
  char _eb[140] = "";
  int _el = 0;
  // список моделей
  char _mid[MAXM][56];
  int _nm = 0;
  int _mshow[MAXM], _nshow = 0, _msel = 0, _mfirst = 0;
  char _mf[24] = "";
  int _mfl = 0;

  // ---------------- чат ----------------
  void onChat(const KeyEvent& e) {
    switch (e.key) {
      case Key::ESC:
        if (_inLen > 0) { _in[0] = 0; _inLen = 0; } else { requestExit(); }
        break;
      case Key::DEL: delLast(_in, _inLen); break;
      case Key::UP:   if (_scroll > 0) { _scroll -= 18; if (_scroll < 0) _scroll = 0; } break;
      case Key::DOWN: if (_scroll < _maxScroll) _scroll += 18; break;
      case Key::LEFT:  switchProfile(-1); break;
      case Key::RIGHT: switchProfile(+1); break;
      case Key::ENTER:
        if (_inLen == 0) { _scr = SET; _sel = 0; _first = 0; _clear = true; }
        else submit();
        break;
      case Key::CHAR:
        if (e.ch == '\\') { _ru = !_ru; break; }
        typeInto(_in, _inLen, (int)sizeof(_in), e.ch, _ru);
        break;
      default: break;
    }
  }

  void switchProfile(int d) {
    _s.active = (_s.active + d + AiSettings::NP) % AiSettings::NP;
    _s.save();
    _hn = 0;
    for (int i = 0; i < HMAX; i++) { _hq[i].clear(); _ha[i].clear(); }
    snprintf(_status, sizeof(_status), "Профиль: %s", _s.cur().name);
  }

  static void delLast(char* b, int& n) {
    if (n <= 0) return;
    do { n--; } while (n > 0 && (((unsigned char)b[n]) & 0xC0) == 0x80);
    b[n] = 0;
  }
  static void typeInto(char* b, int& n, int cap, char ch, bool ru) {
    if (ch < 32 || ch >= 127) return;
    char t[4];
    if (ru) aiu::ruKey(ch, t); else { t[0] = ch; t[1] = 0; }
    int l = (int)strlen(t);
    if (n + l < cap - 1) { memcpy(b + n, t, l); n += l; b[n] = 0; }
  }

  void submit() {
    if (!strcmp(_in, "/new") || !strcmp(_in, ".туц")) {   // команда работает в любой раскладке
      for (int i = 0; i < HMAX; i++) { _hq[i].clear(); _ha[i].clear(); }
      _hn = 0; _q.clear(); _a.clear(); _status[0] = 0; _in[0] = 0; _inLen = 0; _clear = true;
      return;
    }
    AiProfile& pf = _s.cur();
    _q.clear(); _q.add(_in);
    _a.clear(); _status[0] = 0;
    _in[0] = 0; _inLen = 0;
    if (!pf.key[0])  { snprintf(_status, sizeof(_status), "Нет ключа API. Enter на пустой строке — настройки."); return; }
    if (!pf.model[0]) { snprintf(_status, sizeof(_status), "Не указана модель. Enter на пустой строке — настройки."); return; }
    if (!pf.base[0]) { snprintf(_status, sizeof(_status), "Не указан адрес API. Enter на пустой строке — настройки."); return; }
    if (WiFi.status() == WL_CONNECTED) { _phase = 2; snprintf(_status, sizeof(_status), "Думаю..."); }
    else { _phase = 1; _next = 2; snprintf(_status, sizeof(_status), "Подключаюсь к Wi-Fi..."); }
    _wait = 1;
  }

  void drawChat() {
    ui::titleBar("Спросить ИИ");
    gfx.setTextColor(RGB565(255,255,255), C_ACCENT);
    char tag[40];
    snprintf(tag, sizeof(tag), "%s  %s", _s.cur().name, _ru ? "RU" : "EN");
    gfx.drawString(tag, SCREEN_W - 8 - gfx.textWidth(tag), 4);

    const int top = ui::TITLE_H + 2, bottom = 174;
    gfx.fillRect(0, top, SCREEN_W, 190 - top, C_BG);
    gfx.setClipRect(0, top, SCREEN_W, bottom - top);
    int y = top + 4 - _scroll;
    if (_q.size() > 0) { y += wrap(_q.c_str(), ui::PAD, y, SCREEN_W - 2 * ui::PAD, C_ACCENT) + 6; }
    if (_status[0]) {
      y += wrap(_status, ui::PAD, y, SCREEN_W - 2 * ui::PAD, C_DIM);
    }
    if (_a.size() > 0) { y += wrap(_a.c_str(), ui::PAD, y, SCREEN_W - 2 * ui::PAD, C_FG); }
    gfx.clearClipRect();
    if (_q.size() == 0 && !_status[0]) {
      gfx.setFont(UI_FONT);
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString("Напиши вопрос, Enter — отправить.", ui::PAD, top + 6);
      gfx.drawString("Enter на пустой строке — настройки.", ui::PAD, top + 26);
      gfx.drawString("\\ раскладка RU/EN, & даёт знак ?", ui::PAD, top + 46);
      gfx.drawString("/new — начать диалог заново.", ui::PAD, top + 66);
    }
    int total = y - (top + 4 - _scroll);
    _maxScroll = total + 8 - (bottom - top);
    if (_maxScroll < 0) _maxScroll = 0;
    // модель
    char m[100], f[100];
    snprintf(m, sizeof(m), "%s: %s", _s.cur().name, _s.cur().model[0] ? _s.cur().model : "(модель не указана)");
    fitText2(m, SCREEN_W - 2 * ui::PAD, f, sizeof(f));
    gfx.setFont(UI_FONT);
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString(f, ui::PAD, 174);
    // ввод
    const int iy = 194;
    gfx.fillRect(0, iy, SCREEN_W, 24, C_SEL);
    gfx.setTextColor(C_ACCENT, C_SEL);
    gfx.drawString(">", 6, iy + 4);
    char show[72];
    fitTail(_in, SCREEN_W - 40, show, sizeof(show));
    gfx.setTextColor(C_FG, C_SEL);
    gfx.drawString(show, 20, iy + 4);
    if (_phase == 0) gfx.drawString("_", 20 + gfx.textWidth(show), iy + 4);
    ui::footer("Enter отпр. \\ RU/EN Fn+,/ профиль");
  }

  // ---------------- настройки ----------------
  const char* rowLabel(int r) const {
    static const char* L[R_COUNT] = {"Профиль", "Название профиля", "Шаблон адреса", "Адрес API", "Ключ API", "Модель",
                                     "Выбрать модель из списка", "Wi-Fi 1: имя сети", "Wi-Fi 1: пароль", "Wi-Fi 2: имя сети", "Wi-Fi 2: пароль", "Импорт из /ai/config.txt"};
    return L[r];
  }

  static void maskKey(const char* k, char* out, size_t cap) {
    size_t n = strlen(k);
    if (n == 0) snprintf(out, cap, "(пусто)");
    else if (n <= 8) snprintf(out, cap, "*** (%d)", (int)n);
    else snprintf(out, cap, "%.4s...%s (%d)", k, k + n - 4, (int)n);
  }

  void rowValue(int r, char* v, size_t cap) {
    AiProfile& pf = _s.cur();
    switch (r) {
      case R_PROFILE:  snprintf(v, cap, "< %d: %s >", _s.active + 1, pf.name); break;
      case R_NAME:     snprintf(v, cap, "%s", pf.name); break;
      case R_TEMPLATE: { int t = templateIndex(); snprintf(v, cap, "< %s >", t >= 0 ? kAiTemplates[t].name : "другой"); } break;
      case R_BASE:     snprintf(v, cap, "%s", pf.base[0] ? pf.base : "(пусто)"); break;
      case R_KEY:      maskKey(pf.key, v, cap); break;
      case R_MODEL:    snprintf(v, cap, "%s", pf.model[0] ? pf.model : "(пусто)"); break;
      case R_PICK:     snprintf(v, cap, "Enter"); break;
      case R_W1:       snprintf(v, cap, "%s", _s.w[0].ssid[0] ? _s.w[0].ssid : "(пусто)"); break;
      case R_P1:       snprintf(v, cap, "%s", _s.w[0].pass[0] ? "****" : "(пусто)"); break;
      case R_W2:       snprintf(v, cap, "%s", _s.w[1].ssid[0] ? _s.w[1].ssid : "(пусто)"); break;
      case R_P2:       snprintf(v, cap, "%s", _s.w[1].pass[0] ? "****" : "(пусто)"); break;
      default:         snprintf(v, cap, "Enter"); break;
    }
  }

  int templateIndex() {
    for (int i = 0; i < kAiTemplateCount; i++) if (!strcmp(_s.cur().base, kAiTemplates[i].base)) return i;
    return -1;
  }

  void onSet(const KeyEvent& e) {
    bool up = e.key == Key::UP, dn = e.key == Key::DOWN, lf = e.key == Key::LEFT, rt = e.key == Key::RIGHT;
    if (e.key == Key::ESC) { _scr = CHAT; _clear = true; _status[0] = 0; return; }
    if (up && _sel > 0) _sel--;
    if (dn && _sel < R_COUNT - 1) _sel++;
    if ((lf || rt) && _sel == R_PROFILE) { int d = rt ? 1 : -1; _s.active = (_s.active + d + AiSettings::NP) % AiSettings::NP; _s.save(); }
    if ((lf || rt) && _sel == R_TEMPLATE) {
      int d = rt ? 1 : -1, t = templateIndex();
      t = (t < 0) ? (rt ? 0 : kAiTemplateCount - 1) : (t + d + kAiTemplateCount) % kAiTemplateCount;
      aiu::setStr(_s.cur().base, sizeof(_s.cur().base), kAiTemplates[t].base);
      if (!_s.cur().name[0] || nameIsTemplate()) aiu::setStr(_s.cur().name, sizeof(_s.cur().name), kAiTemplates[t].name);
      _s.save();
    }
    if (e.key == Key::ENTER) enterRow(_sel);
  }

  bool nameIsTemplate() {
    for (int i = 0; i < kAiTemplateCount; i++) if (!strcmp(_s.cur().name, kAiTemplates[i].name)) return true;
    return false;
  }

  void enterRow(int r) {
    AiProfile& pf = _s.cur();
    const char* src = nullptr; int cap = 0;
    switch (r) {
      case R_NAME:  src = pf.name;  cap = sizeof(pf.name);  break;
      case R_BASE:  src = pf.base;  cap = sizeof(pf.base);  break;
      case R_KEY:   src = pf.key;   cap = sizeof(pf.key);   break;
      case R_MODEL: src = pf.model; cap = sizeof(pf.model); break;
      case R_W1: src = _s.w[0].ssid; cap = sizeof(_s.w[0].ssid); break;
      case R_P1: src = _s.w[0].pass; cap = sizeof(_s.w[0].pass); break;
      case R_W2: src = _s.w[1].ssid; cap = sizeof(_s.w[1].ssid); break;
      case R_P2: src = _s.w[1].pass; cap = sizeof(_s.w[1].pass); break;
      case R_PICK: startModels(); return;
      case R_IMPORT: importFromSd(); return;
      default: return;
    }
    aiu::setStr(_eb, sizeof(_eb), src);
    _el = (int)strlen(_eb);
    _editRow = r; _editCap = cap;
    _ruEdit = false;                           // в полях ввода по умолчанию латиница
    _scr = EDIT; _clear = true;
  }

  void importFromSd() {
    if (!g_sdOk) { snprintf(_status, sizeof(_status), "SD-карта не найдена"); return; }
    File f = SD.open(AI_CONFIG_PATH, FILE_READ);
    if (!f) { snprintf(_status, sizeof(_status), "Нет файла /ai/config.txt"); return; }
    aiu::Buf txt;
    uint8_t buf[256];
    int n;
    while ((n = f.read(buf, sizeof(buf))) > 0) txt.addn((const char*)buf, (size_t)n);
    f.close();
    aiu::Cfg c;
    aiu::parseConfig(txt.c_str(), c);
    _s.importCfg(c);
    _s.save();
    snprintf(_status, sizeof(_status), "Импортировано из config.txt");
  }

  void drawSet() {
    ui::titleBar("Настройки ИИ");
    int vis = ui::visibleRows();
    if (_sel < _first) _first = _sel;
    if (_sel >= _first + vis) _first = _sel - vis + 1;
    char v[160], fv[80];
    for (int i = 0; i < vis; i++) {
      int idx = _first + i;
      int y = ui::LIST_TOP + i * ui::ROW_H;
      if (idx < R_COUNT) {
        rowValue(idx, v, sizeof(v));
        fitTail(v, SCREEN_W - 20 - gfx.textWidth(rowLabel(idx)) - 12, fv, sizeof(fv));
        ui::row(y, rowLabel(idx), fv, idx == _sel);
      } else gfx.fillRect(0, y, SCREEN_W, ui::ROW_H, C_BG);
    }
    gfx.fillRect(0, ui::FOOT_Y, SCREEN_W, 18, C_BG);
    if (_status[0]) { gfx.setFont(UI_FONT); gfx.setTextColor(C_OK, C_BG); gfx.drawString(_status, 4, ui::FOOT_Y); }
    else ui::footer("Enter изменить  Fn+,/ знач.  Fn+`");
  }

  // ---------------- редактор строки ----------------
  void onEdit(const KeyEvent& e) {
    switch (e.key) {
      case Key::ESC: _scr = SET; _clear = true; return;
      case Key::ENTER: commitEdit(); _scr = SET; _clear = true; return;
      case Key::DEL: delLast(_eb, _el); return;
      case Key::CHAR:
        if (e.ch == '\\' && _editRow != R_BASE && _editRow != R_KEY && _editRow != R_MODEL) { _ruEdit = !_ruEdit; return; }
        typeInto(_eb, _el, _editCap, e.ch, _ruEdit && _editRow != R_BASE && _editRow != R_KEY && _editRow != R_MODEL);
        return;
      default: return;
    }
  }

  void commitEdit() {
    AiProfile& pf = _s.cur();
    switch (_editRow) {
      case R_NAME:  aiu::setStr(pf.name, sizeof(pf.name), _eb); break;
      case R_BASE: {
        aiu::setStr(pf.base, sizeof(pf.base), _eb);
        size_t l = strlen(pf.base);
        while (l > 0 && pf.base[l - 1] == '/') pf.base[--l] = 0;
        break;
      }
      case R_KEY:   aiu::setStr(pf.key, sizeof(pf.key), _eb); break;
      case R_MODEL: aiu::setStr(pf.model, sizeof(pf.model), _eb); break;
      case R_W1: aiu::setStr(_s.w[0].ssid, sizeof(_s.w[0].ssid), _eb); break;
      case R_P1: aiu::setStr(_s.w[0].pass, sizeof(_s.w[0].pass), _eb); break;
      case R_W2: aiu::setStr(_s.w[1].ssid, sizeof(_s.w[1].ssid), _eb); break;
      case R_P2: aiu::setStr(_s.w[1].pass, sizeof(_s.w[1].pass), _eb); break;
    }
    _s.save();
  }

  void drawEdit() {
    ui::titleBar(rowLabel(_editRow));
    gfx.setFont(UI_FONT);
    int cpl = 1;                                    // символов в строке (по ширине "W")
    int cw = gfx.textWidth("W");
    cpl = (SCREEN_W - 2 * ui::PAD) / (cw > 0 ? cw : 8);
    if (cpl < 8) cpl = 8;
    gfx.fillRect(0, ui::LIST_TOP, SCREEN_W, ui::FOOT_Y - ui::LIST_TOP, C_BG);
    gfx.fillRect(ui::PAD - 4, ui::LIST_TOP + 4, SCREEN_W - 2 * ui::PAD + 8, 100, C_SEL);
    gfx.setTextColor(C_FG, C_SEL);
    // построчный вывод с переносом по символам (ключи и адреса без пробелов)
    char line[96];
    const char* p = _eb;
    int y = ui::LIST_TOP + 8;
    while (true) {
      int n = 0, cnt = 0;
      while (p[n] && cnt < cpl && n < (int)sizeof(line) - 5) {
        int len = 1; unsigned char c = (unsigned char)p[n];
        if (c >= 0xF0) len = 4; else if (c >= 0xE0) len = 3; else if (c >= 0xC0) len = 2;
        n += len; cnt++;
      }
      memcpy(line, p, n); line[n] = 0;
      bool last = (p[n] == 0);
      gfx.drawString(line, ui::PAD, y);
      if (last) { gfx.setTextColor(C_ACCENT, C_SEL); gfx.drawString("_", ui::PAD + gfx.textWidth(line), y); break; }
      p += n; y += 18;
      if (y > ui::LIST_TOP + 90) break;
    }
    gfx.setTextColor(C_DIM, C_BG);
    char info[48];
    snprintf(info, sizeof(info), "%d / %d  %s", _el, _editCap - 1, _ruEdit ? "RU" : "EN");
    gfx.drawString(info, ui::PAD, ui::LIST_TOP + 112);
    ui::footer(_editRow == R_BASE || _editRow == R_KEY || _editRow == R_MODEL ? "Enter сохранить  Fn+` отмена" : "Enter сохранить  \\ RU/EN  Fn+`");
  }

  // ---------------- список моделей ----------------
  void startModels() {
    AiProfile& pf = _s.cur();
    if (!pf.base[0]) { snprintf(_status, sizeof(_status), "Сначала укажи адрес API"); return; }
    _nm = 0; _nshow = 0; _msel = 0; _mfirst = 0; _mf[0] = 0; _mfl = 0;
    _scr = MODELS; _clear = true;
    if (WiFi.status() == WL_CONNECTED) { _phase = 3; snprintf(_status, sizeof(_status), "Загружаю список моделей..."); }
    else { _phase = 1; _next = 3; snprintf(_status, sizeof(_status), "Подключаюсь к Wi-Fi..."); }
    _wait = 1;
  }

  static void modelCb(void* ctx, const char* id) { ((AiChatApp*)ctx)->addModel(id); }
  void addModel(const char* id) {
    if (_nm >= MAXM) return;
    if (_nm > 0 && !strncmp(_mid[_nm - 1], id, sizeof(_mid[0]) - 1)) return;
    aiu::setStr(_mid[_nm], sizeof(_mid[0]), id);
    _nm++;
  }

  struct ModelSink : public Stream {
    aiu::IdScanner sc;
    ModelSink(aiu::IdScanner::Cb cb, void* ctx) : sc(cb, ctx) {}
    size_t write(uint8_t c) override { sc.feed(&c, 1); return 1; }
    size_t write(const uint8_t* b, size_t n) override { sc.feed(b, n); return n; }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
  };

  void fetchModels() {
    AiProfile& pf = _s.cur();
    char url[170];
    snprintf(url, sizeof(url), "%s/models", pf.base);
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(25000);
    _status[0] = 0;
    if (!http.begin(client, String(url))) { snprintf(_status, sizeof(_status), "Неверный адрес API"); return; }
    if (pf.key[0]) { char auth[170]; snprintf(auth, sizeof(auth), "Bearer %s", pf.key); http.addHeader("Authorization", String(auth)); }
    int code = http.GET();
    if (code == 200) {
      ModelSink sink(modelCb, this);
      http.writeToStream(&sink);
    }
    http.end();
    if (code < 0) snprintf(_status, sizeof(_status), "Ошибка сети: %s", HTTPClient::errorToString(code).c_str());
    else if (code != 200) snprintf(_status, sizeof(_status), "%s (%d)", code == 401 ? "Неверный ключ" : code == 404 ? "Адрес не найден (у провайдера нет списка)" : "Ошибка", code);
    else if (_nm == 0) snprintf(_status, sizeof(_status), "Список пуст");
    refilter();
  }

  void refilter() {
    _nshow = 0;
    for (int i = 0; i < _nm; i++) if (!_mfl || containsCI(_mid[i], _mf)) _mshow[_nshow++] = i;
    if (_msel >= _nshow) _msel = _nshow ? _nshow - 1 : 0;
    _mfirst = 0;
  }
  static bool containsCI(const char* h, const char* n) {
    size_t ln = strlen(n);
    for (; *h; h++) if (!strncasecmp(h, n, ln)) return true;
    return false;
  }

  void onModels(const KeyEvent& e) {
    switch (e.key) {
      case Key::ESC: _scr = SET; _clear = true; _status[0] = 0; return;
      case Key::UP:   if (_msel > 0) _msel--; return;
      case Key::DOWN: if (_msel < _nshow - 1) _msel++; return;
      case Key::DEL:  delLast(_mf, _mfl); refilter(); return;
      case Key::ENTER:
        if (_nshow > 0) {
          aiu::setStr(_s.cur().model, sizeof(_s.cur().model), _mid[_mshow[_msel]]);
          _s.save();
          _scr = SET; _clear = true; snprintf(_status, sizeof(_status), "Модель выбрана");
        }
        return;
      case Key::CHAR:
        if (e.ch >= 32 && e.ch < 127 && _mfl < (int)sizeof(_mf) - 1) { _mf[_mfl++] = e.ch; _mf[_mfl] = 0; refilter(); }
        return;
      default: return;
    }
  }

  void drawModels() {
    char t[48];
    snprintf(t, sizeof(t), "Модели: %s", _s.cur().name);
    ui::titleBar(t);
    int vis = ui::visibleRows() - 1;
    if (_msel < _mfirst) _mfirst = _msel;
    if (_msel >= _mfirst + vis) _mfirst = _msel - vis + 1;
    // строка фильтра
    gfx.fillRect(0, ui::LIST_TOP, SCREEN_W, ui::ROW_H, C_SEL);
    gfx.setFont(UI_FONT);
    gfx.setTextColor(C_ACCENT, C_SEL);
    gfx.drawString("Поиск:", ui::PAD, ui::LIST_TOP + 3);
    gfx.setTextColor(C_FG, C_SEL);
    char fb[40];
    snprintf(fb, sizeof(fb), "%s_", _mf);
    gfx.drawString(fb, ui::PAD + 56, ui::LIST_TOP + 3);
    char cnt[24];
    snprintf(cnt, sizeof(cnt), "%d/%d", _nshow, _nm);
    gfx.setTextColor(C_DIM, C_SEL);
    gfx.drawString(cnt, SCREEN_W - 8 - gfx.textWidth(cnt), ui::LIST_TOP + 3);
    if (_phase != 0 || _status[0]) {
      gfx.fillRect(0, ui::LIST_TOP + ui::ROW_H, SCREEN_W, ui::FOOT_Y - ui::LIST_TOP - ui::ROW_H, C_BG);
      gfx.setTextColor(_phase != 0 ? C_DIM : C_ERR, C_BG);
      wrap(_status, ui::PAD, ui::LIST_TOP + ui::ROW_H + 8, SCREEN_W - 2 * ui::PAD, _phase != 0 ? C_DIM : C_ERR);
    } else {
      char lab[80];
      for (int i = 0; i < vis; i++) {
        int idx = _mfirst + i;
        int y = ui::LIST_TOP + (i + 1) * ui::ROW_H;
        if (idx < _nshow) { fitText2(_mid[_mshow[idx]], SCREEN_W - 24, lab, sizeof(lab)); ui::row(y, lab, "", idx == _msel); }
        else gfx.fillRect(0, y, SCREEN_W, ui::ROW_H, C_BG);
      }
    }
    ui::footer("Enter выбрать  пиши — поиск  Fn+`");
  }

  // ---------------- сеть ----------------
  bool connectWifi() {
    WiFi.mode(WIFI_STA);
    bool any = false;
    for (int i = 0; i < AiSettings::NW; i++) {
      if (!_s.w[i].ssid[0]) continue;
      any = true;
      WiFi.begin(_s.w[i].ssid, _s.w[i].pass);
      uint32_t t0 = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - t0 < 9000) delay(100);
      if (WiFi.status() == WL_CONNECTED) return true;
      WiFi.disconnect();
    }
    snprintf(_status, sizeof(_status), any ? "Нет Wi-Fi. Включи раздачу на телефоне." : "Не указана сеть Wi-Fi. Enter на пустой строке — настройки.");
    return false;
  }
  void wifiOff() { WiFi.disconnect(true, false); WiFi.mode(WIFI_OFF); }

  void ask() {
    AiProfile& pf = _s.cur();
    aiu::Buf body;
    body.add("{\"model\":\""); aiu::jsonEscapeAdd(body, pf.model);
    body.add("\",\"max_tokens\":500,\"temperature\":0.5,\"messages\":[");
    body.add("{\"role\":\"system\",\"content\":\""); aiu::jsonEscapeAdd(body, kSystem); body.add("\"}");
    for (int i = 0; i < _hn; i++) {
      body.add(",{\"role\":\"user\",\"content\":\"");      aiu::jsonEscapeAdd(body, _hq[i].c_str()); body.add("\"}");
      body.add(",{\"role\":\"assistant\",\"content\":\""); aiu::jsonEscapeAdd(body, _ha[i].c_str()); body.add("\"}");
    }
    body.add(",{\"role\":\"user\",\"content\":\""); aiu::jsonEscapeAdd(body, _q.c_str()); body.add("\"}]}");

    char url[170], auth[170];
    snprintf(url, sizeof(url), "%s/chat/completions", pf.base);
    snprintf(auth, sizeof(auth), "Bearer %s", pf.key);

    WiFiClientSecure client;
    client.setInsecure();            // без проверки сертификата; трафик всё равно шифруется
    HTTPClient http;
    http.setTimeout(25000);
    _status[0] = 0;
    if (!http.begin(client, String(url))) { snprintf(_status, sizeof(_status), "Неверный адрес API"); return; }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String(auth));
    int code = http.POST(String(body.c_str()));
    String resp = http.getString();
    http.end();

    if (code < 0) { snprintf(_status, sizeof(_status), "Ошибка сети: %s", HTTPClient::errorToString(code).c_str()); return; }
    if (code != 200) {
      aiu::Buf msg;
      aiu::jsonGetString(resp.c_str(), "message", nullptr, msg);
      const char* hint = code == 401 ? "Неверный ключ" : code == 403 ? "Доступ запрещён" : code == 404 ? "Модель или адрес не найдены" :
                         code == 429 ? "Лимит запросов, подожди" : code >= 500 ? "Сервер ИИ недоступен" : "Ошибка";
      snprintf(_status, sizeof(_status), "%s (%d). Можно сменить профиль: Fn+, / Fn+/", hint, code);
      _a.add(msg.c_str());
      return;
    }
    const char* from = strstr(resp.c_str(), "\"message\"");
    aiu::Buf ans;
    if (!aiu::jsonGetString(resp.c_str(), "content", from, ans) || ans.size() == 0) { snprintf(_status, sizeof(_status), "Пустой ответ"); return; }
    aiu::stripMarkdown(ans);
    _a.add(ans.c_str());
    if (_hn == HMAX) {
      for (int i = 0; i + 1 < HMAX; i++) { _hq[i].clear(); _hq[i].add(_hq[i + 1].c_str()); _ha[i].clear(); _ha[i].add(_ha[i + 1].c_str()); }
      _hn--;
    }
    _hq[_hn].clear(); _hq[_hn].add(_q.c_str());
    _ha[_hn].clear(); _ha[_hn].add(_a.c_str());
    _hn++;
  }

  static constexpr const char* kSystem =
    "Ты школьный помощник. Отвечай по-русски, кратко и понятно, 3-6 предложений. "
    "Без Markdown и без списков со звёздочками. Формулы пиши простым текстом.";

  // ---------------- текст ----------------
  int wrap(const char* text, int x, int y0, int maxW, uint16_t color) {
    gfx.setFont(UI_FONT);
    gfx.setTextColor(color, C_BG);
    const int lineH = 18;
    int space = gfx.textWidth(" ");
    int cx = x, y = y0;
    const char* p = text;
    char word[100];
    while (*p) {
      if (*p == '\n') { y += lineH; cx = x; p++; continue; }
      if (*p == ' ') { p++; continue; }
      int n = 0;
      while (*p && *p != ' ' && *p != '\n' && n < (int)sizeof(word) - 4) {
        int len = 1; unsigned char c = (unsigned char)*p;
        if (c >= 0xF0) len = 4; else if (c >= 0xE0) len = 3; else if (c >= 0xC0) len = 2;
        for (int k = 0; k < len && *p; k++) word[n++] = *p++;
      }
      word[n] = 0;
      int w = gfx.textWidth(word);
      if (cx > x && cx + w > x + maxW) { cx = x; y += lineH; }
      gfx.drawString(word, cx, y);
      cx += w + space;
    }
    return (y - y0) + lineH;
  }

  // хвост строки, который влезает в maxPx
  static void fitTail(const char* s, int maxPx, char* out, size_t cap) {
    int n = (int)strlen(s), st = 0;
    while (st < n) {
      if (((unsigned char)s[st] & 0xC0) != 0x80 && gfx.textWidth(s + st) <= maxPx) break;
      st++;
    }
    strncpy(out, s + st, cap - 1); out[cap - 1] = 0;
  }
  // начало строки, которое влезает в maxPx
  static void fitText2(const char* s, int maxPx, char* out, size_t cap) {
    strncpy(out, s, cap - 1); out[cap - 1] = 0;
    int n = (int)strlen(out);
    while (n > 0 && gfx.textWidth(out) > maxPx) { n--; while (n > 0 && (((unsigned char)out[n]) & 0xC0) == 0x80) n--; out[n] = 0; }
  }
};
inline App* createAiChat() { return new AiChatApp(); }
