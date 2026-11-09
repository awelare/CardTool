#pragma once
#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include "../app.h"
#include "../ui.h"
#include "../display.h"

class RefbookApp : public App {
public:
  RefbookApp(const char* path, const char* title) : _path(path), _title(title) {}
  void start() override {
    _lvl = 0; _sel = 0; _scr = 0; _dscr = 0; _subject = ""; _theme = ""; _err = false;
    File f = SD.open(_path);
    if (!f) { _err = true; invalidate(); return; }
    if (deserializeJson(_doc, f)) { _err = true; }
    f.close(); invalidate();
  }
  void onKey(const KeyEvent& e) override {
    if (_err) { if (e.key == Key::ESC) requestExit(); return; }
    if (e.key == Key::ESC) {
      if (_lvl == 0) { requestExit(); return; }
      if (_lvl == 3) { _lvl = 2; _dscr = 0; invalidate(); return; }
      _lvl--; _sel = 0; _scr = 0; invalidate(); return;
    }
    if (_lvl == 3) {
      if (e.key == Key::UP || e.ch == ';')   { if (_dscr > 0) { _dscr--; invalidate(); } return; }
      if (e.key == Key::DOWN || e.ch == '.') { _dscr++; invalidate(); return; }
      return;
    }
    int n = count();
    if (e.key == Key::UP || e.ch == ';')   { if (_sel > 0) { _sel--; vis(); invalidate(); } return; }
    if (e.key == Key::DOWN || e.ch == '.') { if (_sel < n - 1) { _sel++; vis(); invalidate(); } return; }
    if (e.key == Key::ENTER && n > 0) {
      if (_lvl == 0) { _subject = keyAt(_sel); _lvl = 1; _sel = 0; _scr = 0; invalidate(); }
      else if (_lvl == 1) { _theme = keyAt(_sel); _lvl = 2; _sel = 0; _scr = 0; invalidate(); }
      else { _lvl = 3; _dscr = 0; invalidate(); }
    }
  }
  void draw() override {
    gfx.startWrite(); gfx.fillScreen(C_BG);
    if (_err) {
      ui::titleBar(_title);
      gfx.setTextColor(C_ERR, C_BG); gfx.drawString("Нет файла / JSON битый", ui::PAD, ui::LIST_TOP + 10);
      gfx.setTextColor(C_DIM, C_BG); gfx.drawString(_path, ui::PAD, ui::LIST_TOP + 28);
      ui::footer("Esc выход"); gfx.endWrite(); return;
    }
    if (_lvl == 3) detail(); else list();
    gfx.endWrite();
  }
private:
  const char* _path; const char* _title;
  JsonDocument _doc;
  int _lvl = 0, _sel = 0, _scr = 0, _dscr = 0;
  bool _err = false;
  String _subject, _theme;

  int count() {
    if (_lvl == 0) return _doc.as<JsonObject>().size();
    if (_lvl == 1) { JsonObject s = _doc[_subject]; return s ? (int)s.size() : 0; }
    if (_lvl == 2) { JsonArray a = _doc[_subject][_theme]; return a ? (int)a.size() : 0; }
    return 0;
  }
  const char* keyAt(int idx) {
    int i = 0;
    if (_lvl == 0) for (JsonPair kv : _doc.as<JsonObject>()) if (i++ == idx) return kv.key().c_str();
    if (_lvl == 1) { JsonObject s = _doc[_subject]; if (s) for (JsonPair kv : s) if (i++ == idx) return kv.key().c_str(); }
    return "";
  }
  void vis() {
    int v = ui::visibleRows();
    if (_sel < _scr) _scr = _sel;
    if (_sel >= _scr + v) _scr = _sel - v + 1;
  }
  void list() {
    char hdr[40];
    if (_lvl == 0) snprintf(hdr, sizeof(hdr), "%s", _title);
    else if (_lvl == 1) snprintf(hdr, sizeof(hdr), "%.28s", _subject.c_str());
    else snprintf(hdr, sizeof(hdr), "%.28s", _theme.c_str());
    ui::titleBar(hdr);
    int n = count(), v = ui::visibleRows();
    if (_lvl <= 1) {
      for (int i = 0; i < v && _scr + i < n; i++)
        ui::row(ui::LIST_TOP + i * ui::ROW_H, keyAt(_scr + i), "", (_scr + i) == _sel);
    } else {
      JsonArray a = _doc[_subject][_theme];
      for (int i = 0; i < v && _scr + i < n; i++) {
        int idx = _scr + i;
        const char* nm = a[idx]["name"] | "?";
        const char* vl = a[idx]["value"] | "";
        ui::row(ui::LIST_TOP + i * ui::ROW_H, nm, vl, idx == _sel);
      }
    }
    if (n == 0) { gfx.setTextColor(C_DIM, C_BG); gfx.drawString("Пусто", ui::PAD, ui::LIST_TOP + 10); }
    ui::footer(_lvl == 0 ? "Стрелки Enter Esc" : "Стрелки Enter Esc назад");
  }
  int wrap(const char* text, int x, int y0, int maxW, int maxY, int skip, uint16_t color) {
    gfx.setTextColor(color, C_BG);
    int line = 0, y = y0;
    const char* p = text;
    while (*p && y < maxY) {
      int n = 0; char buf[100];
      while (p[n] && n < 99) { buf[n] = p[n]; buf[n+1] = 0; if (gfx.textWidth(buf) > maxW) { buf[n] = 0; break; } n++; }
      if (!n) break;
      if (line >= skip) { gfx.drawString(buf, x, y); y += 14; }
      line++; p += n; while (*p == ' ') p++;
    }
    return y;
  }
  void detail() {
    JsonObject it = _doc[_subject][_theme][_sel];
    ui::titleBar(it["name"] | "?");
    gfx.setTextSize(1);
    int maxW = SCREEN_W - 2 * ui::PAD, maxY = ui::FOOT_Y - 4;
    int y = wrap(it["value"] | "", ui::PAD, ui::LIST_TOP, maxW, maxY, 0, C_ACCENT);
    y += 4;
    wrap(it["desc"] | "", ui::PAD, y, maxW, maxY, _dscr, C_FG);
    ui::footer("Вверх/вниз скролл  Esc назад");
  }
};
inline App* createFormulas()    { return new RefbookApp("/school/formulas.json",    "Формулы"); }
inline App* createDefinitions() { return new RefbookApp("/school/definitions.json", "Определения"); }