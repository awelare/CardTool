#pragma once
#include <stdio.h>
#include <string.h>
#include <SD.h>
#include "../app.h"
#include "../ui.h"
#include "../storage.h"

class FileManagerApp : public App {
public:
  void start() override {
    strcpy(_path, "/");
    _sel = 0; _first = 0; _scr = LIST;
    rescan();
  }
  void stop() override {
    if (_buf) { free(_buf); _buf = nullptr; }
  }

  void onKey(const KeyEvent& e) override {
    if (_scr == LIST)    { onList(e);    return; }
    if (_scr == VIEW)    { onView(e);    return; }
    if (_scr == INFO)    { onInfo(e);    return; }
    if (_scr == CONFIRM) { onConfirm(e); return; }
    if (_scr == EDIT)    { onInput(e);   return; }
  }

  void draw() override {
    gfx.startWrite();
    gfx.fillScreen(C_BG);
    if (_scr == LIST) drawList();
    else if (_scr == VIEW) drawView();
    else if (_scr == INFO) drawInfo();
    else if (_scr == CONFIRM) drawConfirm();
    else if (_scr == EDIT) drawInput();
    gfx.endWrite();
  }

private:
  enum Scr : uint8_t { LIST, VIEW, INFO, CONFIRM, EDIT };
  enum Op  : uint8_t { OP_DELETE, OP_RENAME, OP_MKDIR };
  struct Entry { char name[48]; bool dir; uint32_t size; };
  static const int MAXE = 100;

  Scr _scr = LIST;
  char _path[128] = "/";
  Entry _it[MAXE];
  int _n = 0, _sel = 0, _first = 0;

  char* _buf = nullptr;
  int _bufLen = 0, _viewOff = 0, _totalLines = 0;
  char _viewName[48];

  char _infoName[48];
  uint32_t _infoSize = 0;
  bool _infoDir = false;

  Op _op = OP_DELETE;
  char _opTarget[48];

  char _input[48];
  int _inputLen = 0;
  Op _inputOp = OP_MKDIR;

  static void fmtSize(uint32_t sz, char* out, size_t cap) {
    if (sz < 1024) snprintf(out, cap, "%u Б", (unsigned)sz);
    else if (sz < 1024UL*1024UL) snprintf(out, cap, "%u КБ", (unsigned)(sz/1024));
    else snprintf(out, cap, "%u МБ", (unsigned)(sz/1024/1024));
  }

  void joinPath(char* out, size_t cap, const char* base, const char* name) {
    if (base[strlen(base)-1] == '/') snprintf(out, cap, "%s%s", base, name);
    else snprintf(out, cap, "%s/%s", base, name);
  }

  void ensureVisible() {
    int vis = ui::visibleRows();
    if (_sel < _first) _first = _sel;
    if (_sel >= _first + vis) _first = _sel - vis + 1;
  }

  void rescan() {
    _n = 0;
    if (!g_sdOk) { if (_sel >= _n) _sel = 0; _first = 0; invalidate(); return; }
    File d = SD.open(_path);
    if (!d || !d.isDirectory()) { if (_sel >= _n) _sel = 0; _first = 0; invalidate(); return; }
    while (_n < MAXE) {
      File f = d.openNextFile();
      if (!f) break;
      const char* nm = f.name();
      if (nm[0] == '.') { f.close(); continue; }
      strncpy(_it[_n].name, nm, sizeof(_it[_n].name) - 1);
      _it[_n].name[sizeof(_it[_n].name)-1] = 0;
      _it[_n].dir = f.isDirectory();
      _it[_n].size = _it[_n].dir ? 0 : (uint32_t)f.size();
      _n++;
      f.close();
    }
    d.close();
    for (int i = 1; i < _n; i++) {
      Entry t = _it[i]; int j = i - 1;
      while (j >= 0 && ((t.dir && !_it[j].dir) ||
             (t.dir == _it[j].dir && strcasecmp(t.name, _it[j].name) < 0))) {
        _it[j+1] = _it[j]; j--;
      }
      _it[j+1] = t;
    }
    if (_sel >= _n) _sel = _n > 0 ? _n - 1 : 0;
    ensureVisible();
    invalidate();
  }

  void onList(const KeyEvent& e) {
    if (e.key == Key::ESC) {
      if (strcmp(_path, "/") == 0) { requestExit(); return; }
      char* p = strrchr(_path, '/');
      if (p == _path) strcpy(_path, "/");
      else *p = 0;
      _sel = 0; _first = 0;
      rescan();
      return;
    }
    bool up = e.key == Key::UP   || (e.key == Key::CHAR && e.ch == ';');
    bool dn = e.key == Key::DOWN || (e.key == Key::CHAR && e.ch == '.');
    if (up && _sel > 0)        { _sel--; ensureVisible(); invalidate(); return; }
    if (dn && _sel < _n - 1)   { _sel++; ensureVisible(); invalidate(); return; }
    if (e.key == Key::ENTER && _n > 0) {
      if (_it[_sel].dir) {
        char np[128];
        joinPath(np, sizeof(np), _path, _it[_sel].name);
        strncpy(_path, np, sizeof(_path)-1);
        _path[sizeof(_path)-1] = 0;
        _sel = 0; _first = 0;
        rescan();
      } else openViewOrInfo(_sel);
      return;
    }
    if (e.key == Key::DEL && _n > 0) {
      _op = OP_DELETE;
      strncpy(_opTarget, _it[_sel].name, sizeof(_opTarget)-1);
      _opTarget[sizeof(_opTarget)-1] = 0;
      _scr = CONFIRM; invalidate(); return;
    }
    if (e.key == Key::CHAR && _n > 0) {
      if (e.ch == 'r') {
        _op = OP_RENAME;
        strncpy(_opTarget, _it[_sel].name, sizeof(_opTarget)-1);
        _opTarget[sizeof(_opTarget)-1] = 0;
        strncpy(_input, _it[_sel].name, sizeof(_input)-1);
        _input[sizeof(_input)-1] = 0;
        _inputLen = (int)strlen(_input);
        _inputOp = OP_RENAME;
        _scr = EDIT; invalidate(); return;
      }
      if (e.ch == 'n') {
        _opTarget[0] = 0; _input[0] = 0; _inputLen = 0;
        _inputOp = OP_MKDIR;
        _scr = EDIT; invalidate(); return;
      }
      if (e.ch == 'i') {
        strncpy(_infoName, _it[_sel].name, sizeof(_infoName)-1);
        _infoName[sizeof(_infoName)-1] = 0;
        _infoSize = _it[_sel].size;
        _infoDir  = _it[_sel].dir;
        _scr = INFO; invalidate(); return;
      }
    }
  }

  void openViewOrInfo(int idx) {
    char full[192];
    joinPath(full, sizeof(full), _path, _it[idx].name);
    File f = SD.open(full);
    if (!f) return;
    size_t sz = f.size();
    bool fallbackInfo = (sz > 8192);
    if (!fallbackInfo) {
      if (_buf) { free(_buf); _buf = nullptr; }
      _buf = (char*)malloc(sz + 1);
      if (!_buf) { fallbackInfo = true; }
      else {
        _bufLen = (int)f.read((uint8_t*)_buf, sz);
        _buf[_bufLen] = 0;
        int check = _bufLen < 256 ? _bufLen : 256;
        for (int i = 0; i < check; i++) if (_buf[i] == 0) { fallbackInfo = true; break; }
      }
    }
    f.close();
    if (fallbackInfo) {
      if (_buf) { free(_buf); _buf = nullptr; }
      strncpy(_infoName, _it[idx].name, sizeof(_infoName)-1);
      _infoName[sizeof(_infoName)-1] = 0;
      _infoSize = _it[idx].size;
      _infoDir = false;
      _scr = INFO; invalidate(); return;
    }
    strncpy(_viewName, _it[idx].name, sizeof(_viewName)-1);
    _viewName[sizeof(_viewName)-1] = 0;
    _totalLines = 1;
    for (int i = 0; i < _bufLen; i++) if (_buf[i] == '\n') _totalLines++;
    _viewOff = 0;
    _scr = VIEW; invalidate();
  }

  void onView(const KeyEvent& e) {
    if (e.key == Key::ESC) {
      if (_buf) { free(_buf); _buf = nullptr; }
      _scr = LIST; invalidate(); return;
    }
    int lineH = gfx.fontHeight();
    int maxLines = (ui::FOOT_Y - ui::LIST_TOP - 2) / lineH;
    bool up = e.key == Key::UP   || (e.key == Key::CHAR && e.ch == ';');
    bool dn = e.key == Key::DOWN || (e.key == Key::CHAR && e.ch == '.');
    if (up && _viewOff > 0) { _viewOff--; invalidate(); }
    if (dn && _viewOff + maxLines < _totalLines) { _viewOff++; invalidate(); }
  }

  void onInfo(const KeyEvent& e) {
    if (e.key == Key::ESC) { _scr = LIST; invalidate(); }
  }

  bool removeRecursive(const char* path) {
    File f = SD.open(path);
    if (!f) return false;
    if (!f.isDirectory()) { f.close(); return SD.remove(path); }
    f.rewindDirectory();
    while (true) {
      File c = f.openNextFile();
      if (!c) break;
      char child[192];
      snprintf(child, sizeof(child), "%s/%s", path, c.name());
      bool isDir = c.isDirectory();
      c.close();
      if (isDir) removeRecursive(child);
      else SD.remove(child);
    }
    f.close();
    return SD.rmdir(path);
  }

  void onConfirm(const KeyEvent& e) {
    if (e.key == Key::ESC) { _scr = LIST; invalidate(); return; }
    if (e.key == Key::ENTER) {
      if (_op == OP_DELETE) {
        char full[192];
        joinPath(full, sizeof(full), _path, _opTarget);
        removeRecursive(full);
      }
      _scr = LIST;
      rescan();
    }
  }

  void onInput(const KeyEvent& e) {
    if (e.key == Key::ESC) { _scr = LIST; invalidate(); return; }
    if (e.key == Key::DEL) {
      if (_inputLen > 0) { _inputLen--; _input[_inputLen] = 0; invalidate(); }
      return;
    }
    if (e.key == Key::ENTER) {
      if (_inputLen > 0) {
        char oldP[192], newP[192];
        if (_inputOp == OP_RENAME && _opTarget[0]) {
          joinPath(oldP, sizeof(oldP), _path, _opTarget);
          joinPath(newP, sizeof(newP), _path, _input);
          SD.rename(oldP, newP);
        } else if (_inputOp == OP_MKDIR) {
          joinPath(newP, sizeof(newP), _path, _input);
          SD.mkdir(newP);
        }
      }
      _scr = LIST;
      rescan();
      return;
    }
    if (e.key == Key::CHAR && e.ch >= 32 && e.ch < 127 && _inputLen < (int)sizeof(_input)-1) {
      if (e.ch == '/' || e.ch == '\\' || e.ch == ':' || e.ch == '*' ||
          e.ch == '?' || e.ch == '"' || e.ch == '<' || e.ch == '>' || e.ch == '|') return;
      _input[_inputLen++] = e.ch;
      _input[_inputLen] = 0;
      invalidate();
    }
  }

  void drawList() {
    if (!g_sdOk) {
      ui::titleBar("Файлы");
      gfx.setTextColor(C_ERR, C_BG);
      gfx.drawString("SD не смонтирована", ui::PAD, ui::LIST_TOP + 8);
      ui::footer("Esc — выход");
      return;
    }
    char t[80];
    if (strcmp(_path, "/") == 0) snprintf(t, sizeof(t), "SD: /");
    else snprintf(t, sizeof(t), "SD: %s", _path);
    ui::titleBar(t);
    int vis = ui::visibleRows();
    for (int i = 0; i < vis; i++) {
      int idx = _first + i;
      int y = ui::LIST_TOP + i * ui::ROW_H;
      if (idx < _n) {
        char val[16];
        if (_it[idx].dir) strcpy(val, "папка");
        else fmtSize(_it[idx].size, val, sizeof(val));
        ui::row(y, _it[idx].name, val, idx == _sel);
      } else gfx.fillRect(0, y, SCREEN_W, ui::ROW_H, C_BG);
    }
    ui::footer("Enter откр  Del  r n i  Esc");
  }

  void drawView() {
    ui::titleBar(_viewName);
    if (!_buf) { ui::footer("Файл пуст"); return; }
    int lineH = gfx.fontHeight();
    int maxLines = (ui::FOOT_Y - ui::LIST_TOP - 2) / lineH;
    int pos = 0, curLine = 0;
    while (pos < _bufLen && curLine < _viewOff) {
      if (_buf[pos] == '\n') curLine++;
      pos++;
    }
    char lineBuf[64];
    for (int i = 0; i < maxLines && pos < _bufLen; i++) {
      int start = pos;
      while (pos < _bufLen && _buf[pos] != '\n' && _buf[pos] != '\r') pos++;
      int len = pos - start;
      if (len > (int)sizeof(lineBuf)-1) len = sizeof(lineBuf)-1;
      memcpy(lineBuf, _buf + start, len);
      lineBuf[len] = 0;
      gfx.setTextColor(C_FG, C_BG);
      gfx.drawString(lineBuf, ui::PAD, ui::LIST_TOP + i * lineH + 2);
      if (pos < _bufLen && _buf[pos] == '\r') pos++;
      if (pos < _bufLen && _buf[pos] == '\n') pos++;
    }
    char foot[64];
    snprintf(foot, sizeof(foot), "Стр %d/%d  ; . скролл  Esc", _viewOff + 1, _totalLines);
    ui::footer(foot);
  }

  void drawInfo() {
    ui::titleBar("Инфо");
    char v[24];
    ui::row(ui::LIST_TOP + 0*ui::ROW_H, "Имя", _infoName, false);
    ui::row(ui::LIST_TOP + 1*ui::ROW_H, "Тип", _infoDir ? "папка" : "файл", false);
    if (!_infoDir) {
      fmtSize(_infoSize, v, sizeof(v));
      ui::row(ui::LIST_TOP + 2*ui::ROW_H, "Размер", v, false);
    }
    ui::footer("Esc — назад");
  }

  void drawConfirm() {
    ui::titleBar("Удалить?");
    gfx.setTextColor(C_FG, C_BG);
    gfx.drawString("Удалить:", ui::PAD, ui::LIST_TOP + 8);
    gfx.setTextColor(C_ERR, C_BG);
    gfx.drawString(_opTarget, ui::PAD, ui::LIST_TOP + 30);
    gfx.setTextColor(C_FG, C_BG);
    gfx.drawString("Безвозвратно!", ui::PAD, ui::LIST_TOP + 70);
    ui::footer("Enter — да   Esc — нет");
  }

  void drawInput() {
    ui::titleBar(_inputOp == OP_RENAME ? "Переименовать" : "Новая папка");
    if (_inputOp == OP_RENAME) {
      gfx.setTextColor(C_DIM, C_BG);
      gfx.drawString("Старое:", ui::PAD, ui::LIST_TOP + 4);
      gfx.setTextColor(C_FG, C_BG);
      gfx.drawString(_opTarget, ui::PAD, ui::LIST_TOP + 22);
    }
    gfx.setTextColor(C_DIM, C_BG);
    gfx.drawString("Имя:", ui::PAD, ui::LIST_TOP + 50);
    gfx.setTextColor(C_FG, C_BG);
    gfx.drawString(_input, ui::PAD, ui::LIST_TOP + 70);
    int cx = ui::PAD + gfx.textWidth(_input);
    gfx.drawFastVLine(cx + 1, ui::LIST_TOP + 68, gfx.fontHeight(), C_ACCENT);
    ui::footer("Enter ОК  Del стереть  Esc");
  }
};

inline App* createFileManager() { return new FileManagerApp(); }