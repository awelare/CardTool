#pragma once
// Список файлов и папок на SD (для музыки и видео) + вспомогательная подгонка текста
#include <SD.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "../ui.h"

#ifndef MUSIC_DIR
#define MUSIC_DIR "/music"
#endif
#ifndef VIDEO_DIR
#define VIDEO_DIR "/video"
#endif

// Обрезает строку так, чтобы она влезла в maxPx пикселей (не режет UTF-8 символы пополам)
inline void fitText(const char* s, int maxPx, char* out, size_t cap) {
  strncpy(out, s, cap - 1);
  out[cap - 1] = 0;
  int n = (int)strlen(out);
  while (n > 0 && gfx.textWidth(out) > maxPx) {
    n--;
    while (n > 0 && (((unsigned char)out[n]) & 0xC0) == 0x80) n--;
    out[n] = 0;
  }
}

class FileList {
public:
  struct Item { char name[48]; bool dir; };
  static const int MAXN = 96;
  Item it[MAXN];
  int n = 0;
  char root[24] = "/";
  char path[96] = "/";

  void init(const char* rootDir) {
    strncpy(root, rootDir, sizeof(root) - 1);
    root[sizeof(root) - 1] = 0;
    strcpy(path, root);
  }
  bool atRoot() const { return strcmp(path, root) == 0; }

  // exts: расширения через запятую, строчными, например "mp3,wav"
  bool scan(const char* exts) {
    n = 0;
    File d = SD.open(path);
    if (!d || !d.isDirectory()) return false;
    while (n < MAXN) {
      File f = d.openNextFile();
      if (!f) break;
      const char* nm = f.name();
      bool dir = f.isDirectory();
      if (nm[0] != '.' && strcasecmp(nm, "System Volume Information") != 0 && (dir || hasExt(nm, exts))) {
        copyName(it[n].name, nm, sizeof(it[n].name));
        it[n].dir = dir;
        n++;
      }
      f.close();
    }
    d.close();
    sort();
    return true;
  }

  bool enter(int i) {
    if (i < 0 || i >= n || !it[i].dir) return false;
    size_t l = strlen(path);
    if (l + strlen(it[i].name) + 2 >= sizeof(path)) return false;
    if (path[l - 1] != '/') strcat(path, "/");
    strcat(path, it[i].name);
    return true;
  }

  bool up() {
    if (atRoot()) return false;
    char* s = strrchr(path, '/');
    if (!s || s == path) { strcpy(path, root); return true; }
    *s = 0;
    if (strlen(path) < strlen(root)) strcpy(path, root);
    return true;
  }

  void fullPath(int i, char* out, size_t cap) const {
    snprintf(out, cap, "%s%s%s", path, path[strlen(path) - 1] == '/' ? "" : "/", it[i].name);
  }

  // название без расширения
  static void baseName(const char* name, char* out, size_t cap) {
    strncpy(out, name, cap - 1);
    out[cap - 1] = 0;
    char* dot = strrchr(out, '.');
    if (dot && dot != out) *dot = 0;
  }

  // индекс следующего/предыдущего файла (не папки) с зацикливанием, -1 если файлов нет
  int nextFile(int from, int dir) const {
    if (n == 0) return -1;
    for (int k = 1; k <= n; k++) {
      int i = ((from + dir * k) % n + n) % n;
      if (!it[i].dir) return i;
    }
    return -1;
  }

private:
  static bool hasExt(const char* nm, const char* exts) {
    const char* dot = strrchr(nm, '.');
    if (!dot) return false;
    dot++;
    const char* p = exts;
    while (*p) {
      const char* e = strchr(p, ',');
      size_t l = e ? (size_t)(e - p) : strlen(p);
      if (strlen(dot) == l && strncasecmp(dot, p, l) == 0) return true;
      if (!e) break;
      p = e + 1;
    }
    return false;
  }
  static void copyName(char* dst, const char* src, size_t cap) {
    strncpy(dst, src, cap - 1);
    dst[cap - 1] = 0;
    size_t l = strlen(dst);
    if (strlen(src) >= cap) {   // обрезали: не оставлять половину UTF-8 символа
      while (l > 0 && (((unsigned char)dst[l - 1]) & 0xC0) == 0x80) l--;
      if (l > 0 && (((unsigned char)dst[l - 1]) & 0x80)) l--;
      dst[l] = 0;
    }
  }
  void sort() {
    for (int i = 1; i < n; i++) {
      Item t = it[i];
      int j = i - 1;
      while (j >= 0 && before(t, it[j])) { it[j + 1] = it[j]; j--; }
      it[j + 1] = t;
    }
  }
  static bool before(const Item& a, const Item& b) {
    if (a.dir != b.dir) return a.dir;
    return strcasecmp(a.name, b.name) < 0;
  }
};
