#include <M5Cardputer.h>
#include "input.h"

bool inputPoll(KeyEvent& e) {
  M5Cardputer.update();
  if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return false;
  auto st = M5Cardputer.Keyboard.keysState();
  e = KeyEvent();
  if (st.enter) { e.key = Key::ENTER; return true; }
  if (st.del)   { e.key = Key::DEL;   return true; }
  if (st.word.empty()) return false;
  char c = st.word[0];
  if (st.fn) {
    switch (c) {
      case ';': e.key = Key::UP;    return true;
      case '.': e.key = Key::DOWN;  return true;
      case ',': e.key = Key::LEFT;  return true;
      case '/': e.key = Key::RIGHT; return true;
      case '`': e.key = Key::ESC;   return true;
    }
  }
  e.key = Key::CHAR;
  e.ch = c;
  return true;
}
