#include "input/Platform.h"

#include <allegro.h>
#include <array>
#include <cstddef>
#include <fcntl.h>
#include <stdexcept>
#include <string>

namespace openfranko::src::systems::input {
namespace {

constexpr int SCANCODE_SHIFT = 8;
constexpr int CHARACTER_MASK = 0xFF;
constexpr int CONTROL_C = 3;
constexpr int LEFT_BUTTON = 1;
constexpr int FIRST_TYPED = ' ';
constexpr int LAST_TYPED = '~';

std::array<bool, KEY_MAX> heldKeys{};

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Platform error: " + cause);
}

Key toKey(int scancode) {
  switch (scancode) {
  case KEY_UP:
    return Key::Up;
  case KEY_DOWN:
    return Key::Down;
  case KEY_LEFT:
    return Key::Left;
  case KEY_RIGHT:
    return Key::Right;
  case KEY_SPACE:
    return Key::Space;
  case KEY_W:
    return Key::W;
  case KEY_A:
    return Key::A;
  case KEY_S:
    return Key::S;
  case KEY_D:
    return Key::D;
  case KEY_DEL:
    return Key::Delete;
  case KEY_F1:
    return Key::F1;
  case KEY_F2:
    return Key::F2;
  case KEY_F3:
    return Key::F3;
  case KEY_F4:
    return Key::F4;
  case KEY_F9:
    return Key::F9;
  case KEY_ESC:
    return Key::Escape;
  default:
    return Key::Other;
  }
}

char typedCharacter(int scancode) {
  if (scancode == KEY_BACKSPACE) {
    return '\b';
  }
  const int character = scancode_to_ascii(scancode);
  if (character == '\r' ||
      (character >= FIRST_TYPED && character <= LAST_TYPED)) {
    return static_cast<char>(character);
  }
  return 0;
}

bool isBreak(int typed) {
  return (typed & CHARACTER_MASK) == CONTROL_C ||
         ((typed >> SCANCODE_SHIFT) == KEY_PAUSE &&
          (key_shifts & KB_CTRL_FLAG) != 0);
}

void receiveKey(ControllerSystem &controller, int scancode, bool pressed) {
  bool &held = heldKeys[static_cast<std::size_t>(scancode)];
  KeyEvent event;
  event.key = toKey(scancode);
  event.code = scancode;
  event.character = typedCharacter(scancode);
  event.pressed = pressed;
  event.repeat = pressed && held;
  held = pressed;
  controller.receiveKey(event);
}

} // namespace

Platform::Platform() {
  if (!_use_lfn(nullptr)) {
    throwError("long file name support is required");
  }
  if (allegro_init() != 0 || install_timer() != 0 || install_keyboard() != 0) {
    throwError(allegro_error);
  }
  three_finger_flag = FALSE;
  install_mouse();
}

Platform::~Platform() { allegro_exit(); }

bool Platform::pollEvents(ControllerSystem &controller) {
  bool open = true;
  while (keypressed()) {
    const int typed = readkey();
    if (isBreak(typed)) {
      open = false;
    }
    receiveKey(controller, typed >> SCANCODE_SHIFT, true);
  }
  for (int scancode = 0; scancode < KEY_MAX; ++scancode) {
    const bool held = key[scancode] != 0;
    if (held != heldKeys[static_cast<std::size_t>(scancode)]) {
      receiveKey(controller, scancode, held);
    }
  }
  controller.receiveMouseButton((mouse_b & LEFT_BUTTON) != 0);
  return open;
}

} // namespace openfranko::src::systems::input
