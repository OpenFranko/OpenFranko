#include "input/Platform.h"

#include "jaguar/DebugOverlay.h"
#include "jaguar/Eeprom.h"
#include "jaguar/Hardware.h"
#include "jaguar/Joypad.h"
#include "jaguar/Runtime.h"
#include "jaguar/VirtualKeyboard.h"

#include <array>
#include <cstddef>

namespace openfranko::src::systems::input {
namespace {

namespace jaguar = systems::jaguar;
namespace keyboard = systems::jaguar::keyboard;

struct Binding {
  Key key;
  uint32_t buttons;
};

constexpr std::array<Binding, 12> BINDINGS = {{
    {Key::Up, jaguar::PAD_UP},
    {Key::Down, jaguar::PAD_DOWN},
    {Key::Left, jaguar::PAD_LEFT},
    {Key::Right, jaguar::PAD_RIGHT},
    {Key::Space, jaguar::PAD_A | jaguar::PAD_B | jaguar::PAD_C},
    {Key::F1, jaguar::PAD_1},
    {Key::F2, jaguar::PAD_2},
    {Key::F3, jaguar::PAD_3},
    {Key::F4, jaguar::PAD_4},
    {Key::F9, jaguar::PAD_9},
    {Key::Escape, jaguar::PAD_STAR | jaguar::PAD_PAUSE},
    {Key::Delete, jaguar::PAD_HASH},
}};

constexpr uint32_t KEYBOARD_BUTTONS =
    jaguar::PAD_UP | jaguar::PAD_DOWN | jaguar::PAD_LEFT | jaguar::PAD_RIGHT |
    jaguar::PAD_A | jaguar::PAD_B | jaguar::PAD_C;
constexpr int TYPED_CODE = 1000;
constexpr char BACKSPACE = 8;
constexpr char RETURN = 13;
constexpr int REPEAT_DELAY = 18;
constexpr int REPEAT_RATE = 5;
constexpr int ROW_STEP = 5;

std::array<bool, BINDINGS.size()> held{};
bool mouseButton = false;
bool overlayToggle = false;
uint32_t previous = 0;
int repeatFrames = 0;
bool autoOpened = false;
KeyMode lastMode = KeyMode::FrontEnd;

void send(ControllerSystem &controller, Key key, int code, char character,
          bool pressed) {
  KeyEvent event;
  event.key = key;
  event.code = code;
  event.character = character;
  event.pressed = pressed;
  controller.receiveKey(event);
}

void type(ControllerSystem &controller, char character) {
  send(controller, Key::Other, TYPED_CODE + character, character, true);
  send(controller, Key::Other, TYPED_CODE + character, character, false);
}

int steps(uint32_t buttons) {
  if (buttons & jaguar::PAD_LEFT) {
    return -1;
  }
  if (buttons & jaguar::PAD_RIGHT) {
    return 1;
  }
  if (buttons & jaguar::PAD_UP) {
    return -ROW_STEP;
  }
  if (buttons & jaguar::PAD_DOWN) {
    return ROW_STEP;
  }
  return 0;
}

void updateKeyboard(ControllerSystem &controller, uint32_t buttons) {
  const KeyMode mode = controller.keyMode();
  if (mode != lastMode) {
    if (mode == KeyMode::NameEntry || mode == KeyMode::CodeEntry) {
      keyboard::setOpen(true);
      autoOpened = true;
    } else if (autoOpened || mode == KeyMode::Game) {
      keyboard::setOpen(false);
      autoOpened = false;
    }
    lastMode = mode;
  }
  const uint32_t pressed = buttons & ~previous;
  if ((pressed & jaguar::PAD_0) && !(buttons & jaguar::PAD_OPTION) &&
      mode != KeyMode::Game) {
    keyboard::setOpen(!keyboard::isOpen());
    autoOpened = false;
  }
  if (!keyboard::isOpen()) {
    return;
  }
  const int move = steps(buttons);
  if (move != 0 && steps(pressed) == move) {
    keyboard::move(move);
    repeatFrames = REPEAT_DELAY;
  } else if (move != 0 && --repeatFrames <= 0) {
    keyboard::move(move);
    repeatFrames = REPEAT_RATE;
  }
  if (pressed & jaguar::PAD_A) {
    type(controller, keyboard::selected());
  }
  if (pressed & jaguar::PAD_B) {
    type(controller, BACKSPACE);
  }
  if (pressed & jaguar::PAD_C) {
    type(controller, RETURN);
    if (mode == KeyMode::NameEntry) {
      keyboard::setOpen(false);
    }
  }
}

} // namespace

Platform::Platform() {
  jaguar::runtime::installVectors();
#ifdef JAGUAR_DEBUG_OVERLAY
  jaguar::overlay::setEnabled(true);
#endif
}

Platform::~Platform() = default;

bool Platform::pollEvents(ControllerSystem &controller) {
  const uint32_t buttons = jaguar::readJoypad();
  updateKeyboard(controller, buttons);
  const uint32_t routed =
      keyboard::isOpen() ? buttons & ~KEYBOARD_BUTTONS : buttons;
  for (std::size_t index = 0; index < BINDINGS.size(); ++index) {
    const bool pressed = (routed & BINDINGS[index].buttons) != 0;
    if (pressed == held[index]) {
      continue;
    }
    held[index] = pressed;
    send(controller, BINDINGS[index].key, static_cast<int>(index) + 1, 0,
         pressed);
  }
  const bool option = (buttons & jaguar::PAD_OPTION) != 0;
  if (option != mouseButton) {
    mouseButton = option;
    controller.receiveMouseButton(option);
  }
  const bool toggle =
      (buttons & jaguar::PAD_OPTION) && (buttons & jaguar::PAD_0);
  if (toggle && !overlayToggle) {
    jaguar::overlay::setEnabled(!jaguar::overlay::isEnabled());
  }
  overlayToggle = toggle;
  previous = buttons;
  jaguar::eeprom::poll();
  return true;
}

} // namespace openfranko::src::systems::input
