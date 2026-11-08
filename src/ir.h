#pragma once
// Низкоуровневая передача ИК: несущая 38 кГц через LEDC, тайминги по esp_timer
#include <Arduino.h>
#include <esp_timer.h>

namespace ir {
inline int g_pin = -1;
inline uint32_t g_freq = 38000;
static const uint32_t DUTY = 85;   // ~33% из 255

inline bool begin(int pin) {
  g_pin = pin;
  g_freq = 38000;
  if (!ledcAttach(pin, g_freq, 8)) { g_pin = -1; return false; }
  ledcWrite(pin, 0);
  return true;
}

inline void end() {
  if (g_pin < 0) return;
  ledcWrite(g_pin, 0);
  ledcDetach(g_pin);
  pinMode(g_pin, OUTPUT);
  digitalWrite(g_pin, LOW);
  g_pin = -1;
}

// d[0] = импульс (mark), d[1] = пауза (space), далее по очереди, значения в мкс
inline void play(const uint16_t* d, int n, uint32_t freq) {
  if (g_pin < 0) return;
  if (freq < 10000 || freq > 60000) freq = 38000;
  if (freq != g_freq) { ledcChangeFrequency(g_pin, freq, 8); g_freq = freq; }
  int64_t t = esp_timer_get_time();
  for (int i = 0; i < n; i++) {
    ledcWrite(g_pin, (i & 1) ? 0 : DUTY);
    t += d[i];
    while (esp_timer_get_time() < t) {}
  }
  ledcWrite(g_pin, 0);
}

// Кадр NEC / Samsung32: заголовок + 32 бита (младший вперёд) + стоп-бит. Возвращает число значений.
inline int buildNEC(uint16_t* d, uint32_t data, bool samsung) {
  int i = 0;
  d[i++] = samsung ? 4500 : 9000;
  d[i++] = 4500;
  for (int b = 0; b < 32; b++) {
    d[i++] = 560;
    d[i++] = ((data >> b) & 1) ? 1690 : 560;
  }
  d[i++] = 560;
  return i;
}
}  // namespace ir
