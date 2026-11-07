#pragma once
#include <stdint.h>

enum class Key : uint8_t { NONE, UP, DOWN, LEFT, RIGHT, ENTER, ESC, DEL, CHAR };

struct KeyEvent {
  Key key = Key::NONE;
  char ch = 0;   // для Key::CHAR
};

// Стрелки: Fn+; Fn+. Fn+, Fn+/   Esc: Fn+`
bool inputPoll(KeyEvent& e);
