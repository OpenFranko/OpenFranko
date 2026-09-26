#include "ControllerSystem.h"

#include <utility>

namespace openfranko::src::systems {
namespace {

constexpr char TYPED_SPACE = ' ';

FunctionKey toFunctionKey(Key key) {
  switch (key) {
  case Key::F1:
    return FunctionKey::F1;
  case Key::F2:
    return FunctionKey::F2;
  case Key::F3:
    return FunctionKey::F3;
  case Key::F4:
    return FunctionKey::F4;
  case Key::Escape:
    return FunctionKey::Escape;
  default:
    return FunctionKey::Other;
  }
}

} // namespace

void ControllerSystem::update() {
  typed.swap(receivedKeys);
  receivedKeys.clear();
  keyEvent = std::exchange(receivedKeyEvent, std::nullopt);
  mouseButtonDown = mouseButtonHeld;
  deleteHeld = isHeld(Key::Delete);

  const bool letters = keyMode == KeyMode::Game;
  const bool up = isHeld(Key::Up) || (letters && isHeld(Key::W));
  const bool down = isHeld(Key::Down) || (letters && isHeld(Key::S));
  const bool left = isHeld(Key::Left) || (letters && isHeld(Key::A));
  const bool right = isHeld(Key::Right) || (letters && isHeld(Key::D));
  states.up = up && !down;
  states.down = down && !up;
  states.left = left && !right;
  states.right = right && !left;
  states.button = keyMode != KeyMode::NameEntry && isHeld(Key::Space);

  if (states.button && !states.up && !states.down && !states.left &&
      !states.right) {
    fireLatched = true;
  }
}

void ControllerSystem::receiveKey(const KeyEvent &event) {
  heldKeys[static_cast<std::size_t>(event.key)] = event.pressed;
  if (!event.repeat && !isJoystickKey(event.key)) {
    receivedKeyEvent = toFunctionKey(event.key);
  }
  if (!event.pressed) {
    typingKeys.erase(event.code);
    return;
  }
  const std::optional<char> character = typedCharacter(event);
  if (!event.repeat) {
    if (character) {
      typingKeys.insert(event.code);
    } else {
      typingKeys.erase(event.code);
    }
  }
  if (character && typingKeys.count(event.code) != 0) {
    receivedKeys += *character;
  }
}

void ControllerSystem::receiveMouseButton(bool pressed) {
  mouseButtonHeld = pressed;
}

void ControllerSystem::setKeyMode(KeyMode mode) { keyMode = mode; }

void ControllerSystem::clearFireLatch() { fireLatched = false; }

bool ControllerSystem::isFireLatched() const { return fireLatched; }

bool ControllerSystem::isMouseButtonDown() const { return mouseButtonDown; }

bool ControllerSystem::isKeyHeld(Key key) const { return isHeld(key); }

bool ControllerSystem::isDeleteHeld() const { return deleteHeld; }

const std::string &ControllerSystem::typedKeys() const { return typed; }

std::optional<char>
ControllerSystem::typedCharacter(const KeyEvent &event) const {
  if (keyMode == KeyMode::Game || event.character == 0) {
    return std::nullopt;
  }
  if (event.character == TYPED_SPACE && keyMode != KeyMode::NameEntry) {
    return std::nullopt;
  }
  return event.character;
}

std::optional<FunctionKey> ControllerSystem::functionKey() const {
  return keyEvent;
}

bool ControllerSystem::isJoystickKey(Key key) const {
  switch (key) {
  case Key::Up:
  case Key::Down:
  case Key::Left:
  case Key::Right:
    return true;
  case Key::Space:
    return keyMode != KeyMode::NameEntry;
  case Key::W:
  case Key::A:
  case Key::S:
  case Key::D:
    return keyMode == KeyMode::Game;
  default:
    return false;
  }
}

bool ControllerSystem::isHeld(Key key) const {
  return heldKeys[static_cast<std::size_t>(key)];
}

int16_t ControllerSystem::joystick() const {
  return static_cast<int16_t>((states.up ? 1 : 0) | (states.down ? 2 : 0) |
                              (states.left ? 4 : 0) | (states.right ? 8 : 0) |
                              (states.button ? 16 : 0));
}

} // namespace openfranko::src::systems
