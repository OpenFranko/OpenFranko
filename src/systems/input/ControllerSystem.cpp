#include "input/ControllerSystem.h"

#include <utility>

namespace openfranko::src::systems::input {
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
  m_typed.swap(m_receivedKeys);
  m_receivedKeys.clear();
  m_keyEvent = std::exchange(m_receivedKeyEvent, std::nullopt);
  m_mouseButtonDown = m_mouseButtonHeld;
  m_deleteHeld = isHeld(Key::Delete);

  const bool letters = m_keyMode == KeyMode::Game;
  const bool up = isHeld(Key::Up) || (letters && isHeld(Key::W));
  const bool down = isHeld(Key::Down) || (letters && isHeld(Key::S));
  const bool left = isHeld(Key::Left) || (letters && isHeld(Key::A));
  const bool right = isHeld(Key::Right) || (letters && isHeld(Key::D));
  states.up = up && !down;
  states.down = down && !up;
  states.left = left && !right;
  states.right = right && !left;
  states.button = m_keyMode != KeyMode::NameEntry && isHeld(Key::Space);

  if (states.button && !states.up && !states.down && !states.left &&
      !states.right) {
    m_fireLatched = true;
  }
}

void ControllerSystem::receiveKey(const KeyEvent &event) {
  m_heldKeys[static_cast<std::size_t>(event.key)] = event.pressed;
  if (!event.repeat && !isJoystickKey(event.key)) {
    m_receivedKeyEvent = toFunctionKey(event.key);
  }
  if (!event.pressed) {
    m_typingKeys.erase(event.code);
    return;
  }
  const std::optional<char> character = typedCharacter(event);
  if (!event.repeat) {
    if (character) {
      m_typingKeys.insert(event.code);
    } else {
      m_typingKeys.erase(event.code);
    }
  }
  if (character && m_typingKeys.count(event.code) != 0) {
    m_receivedKeys += *character;
  }
}

void ControllerSystem::receiveMouseButton(bool pressed) {
  m_mouseButtonHeld = pressed;
}

void ControllerSystem::setKeyMode(KeyMode mode) { m_keyMode = mode; }

void ControllerSystem::clearFireLatch() { m_fireLatched = false; }

bool ControllerSystem::isFireLatched() const { return m_fireLatched; }

bool ControllerSystem::isMouseButtonDown() const { return m_mouseButtonDown; }

bool ControllerSystem::isKeyHeld(Key key) const { return isHeld(key); }

bool ControllerSystem::isDeleteHeld() const { return m_deleteHeld; }

const std::string &ControllerSystem::typedKeys() const { return m_typed; }

std::optional<char>
ControllerSystem::typedCharacter(const KeyEvent &event) const {
  if (m_keyMode == KeyMode::Game || event.character == 0) {
    return std::nullopt;
  }
  if (event.character == TYPED_SPACE && m_keyMode != KeyMode::NameEntry) {
    return std::nullopt;
  }
  return event.character;
}

std::optional<FunctionKey> ControllerSystem::functionKey() const {
  return m_keyEvent;
}

bool ControllerSystem::isJoystickKey(Key key) const {
  switch (key) {
  case Key::Up:
  case Key::Down:
  case Key::Left:
  case Key::Right:
    return true;
  case Key::Space:
    return m_keyMode != KeyMode::NameEntry;
  case Key::W:
  case Key::A:
  case Key::S:
  case Key::D:
    return m_keyMode == KeyMode::Game;
  default:
    return false;
  }
}

bool ControllerSystem::isHeld(Key key) const {
  return m_heldKeys[static_cast<std::size_t>(key)];
}

int16_t ControllerSystem::joystick() const {
  return static_cast<int16_t>((states.up ? 1 : 0) | (states.down ? 2 : 0) |
                              (states.left ? 4 : 0) | (states.right ? 8 : 0) |
                              (states.button ? 16 : 0));
}

} // namespace openfranko::src::systems::input
