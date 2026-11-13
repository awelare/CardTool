#pragma once
// Настройки ИИ-чата: профили провайдеров и Wi-Fi, хранятся в памяти устройства (NVS)
#include <Preferences.h>
#include <string.h>
#include <stdio.h>
#include "ai_util.h"

struct AiProfile { char name[20]; char base[96]; char key[132]; char model[72]; };
struct AiWifi    { char ssid[40]; char pass[68]; };

struct AiTemplate { const char* name; const char* base; };
// Адреса OpenAI-совместимых API (проверь актуальность на сайте провайдера)
static const AiTemplate kAiTemplates[] = {
  {"Groq",       "https://api.groq.com/openai/v1"},
  {"NVIDIA",     "https://integrate.api.nvidia.com/v1"},
  {"OpenRouter", "https://openrouter.ai/api/v1"},
  {"Cerebras",   "https://api.cerebras.ai/v1"},
};
static const int kAiTemplateCount = sizeof(kAiTemplates) / sizeof(kAiTemplates[0]);

struct AiSettings {
  static const int NP = 4, NW = 2;
  AiProfile p[NP];
  AiWifi w[NW];
  int active = 0;

  void defaults() {
    memset(this, 0, sizeof(*this));
    aiu::setStr(p[0].name, sizeof(p[0].name), "Groq");
    aiu::setStr(p[0].base, sizeof(p[0].base), kAiTemplates[0].base);
    aiu::setStr(p[0].model, sizeof(p[0].model), "llama-3.3-70b-versatile");
    aiu::setStr(p[1].name, sizeof(p[1].name), "NVIDIA");
    aiu::setStr(p[1].base, sizeof(p[1].base), kAiTemplates[1].base);
    aiu::setStr(p[1].model, sizeof(p[1].model), "meta/llama-3.3-70b-instruct");
    aiu::setStr(p[2].name, sizeof(p[2].name), "OpenRouter");
    aiu::setStr(p[2].base, sizeof(p[2].base), kAiTemplates[2].base);
    aiu::setStr(p[3].name, sizeof(p[3].name), "Свой");
    active = 0;
  }

  void load() {
    defaults();
    Preferences pr;
    if (!pr.begin("aichat", false)) return;
    char k[8];
    for (int i = 0; i < NP; i++) {
      snprintf(k, sizeof(k), "p%dn", i); get(pr, k, p[i].name, sizeof(p[i].name));
      snprintf(k, sizeof(k), "p%db", i); get(pr, k, p[i].base, sizeof(p[i].base));
      snprintf(k, sizeof(k), "p%dk", i); get(pr, k, p[i].key, sizeof(p[i].key));
      snprintf(k, sizeof(k), "p%dm", i); get(pr, k, p[i].model, sizeof(p[i].model));
    }
    for (int i = 0; i < NW; i++) {
      snprintf(k, sizeof(k), "w%ds", i); get(pr, k, w[i].ssid, sizeof(w[i].ssid));
      snprintf(k, sizeof(k), "w%dp", i); get(pr, k, w[i].pass, sizeof(w[i].pass));
    }
    int a = pr.getUChar("act", 0);
    active = (a >= 0 && a < NP) ? a : 0;
    pr.end();
  }

  void save() const {
    Preferences pr;
    if (!pr.begin("aichat", false)) return;
    char k[8];
    for (int i = 0; i < NP; i++) {
      snprintf(k, sizeof(k), "p%dn", i); pr.putString(k, p[i].name);
      snprintf(k, sizeof(k), "p%db", i); pr.putString(k, p[i].base);
      snprintf(k, sizeof(k), "p%dk", i); pr.putString(k, p[i].key);
      snprintf(k, sizeof(k), "p%dm", i); pr.putString(k, p[i].model);
    }
    for (int i = 0; i < NW; i++) {
      snprintf(k, sizeof(k), "w%ds", i); pr.putString(k, w[i].ssid);
      snprintf(k, sizeof(k), "w%dp", i); pr.putString(k, w[i].pass);
    }
    pr.putUChar("act", (uint8_t)active);
    pr.end();
  }

  AiProfile& cur() { return p[active]; }

  // Импорт из текстового файла /ai/config.txt (необязательно)
  void importCfg(const aiu::Cfg& c) {
    AiProfile& pf = cur();
    if (c.base[0]) aiu::setStr(pf.base, sizeof(pf.base), c.base);
    if (c.key[0]) aiu::setStr(pf.key, sizeof(pf.key), c.key);
    if (c.model[0]) aiu::setStr(pf.model, sizeof(pf.model), c.model);
    for (int i = 0; i < NW && i < c.nnet; i++) {
      aiu::setStr(w[i].ssid, sizeof(w[i].ssid), c.ssid[i]);
      aiu::setStr(w[i].pass, sizeof(w[i].pass), c.pass[i]);
    }
  }

private:
  static void get(Preferences& pr, const char* key, char* dst, size_t cap) {
    char tmp[200];
    size_t n = pr.getString(key, tmp, sizeof(tmp));
    if (n > 0 || pr.isKey(key)) { tmp[sizeof(tmp) - 1] = 0; aiu::setStr(dst, cap, tmp); }
  }
};
