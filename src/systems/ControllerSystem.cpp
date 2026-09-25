#include "ControllerSystem.h"
#include <cstddef>
#include <utility>

namespace openfranko::src::systems {
namespace {

constexpr SDL_Keycode FIRST_PRINTABLE = '!';
constexpr SDL_Keycode LAST_PRINTABLE = '~';

} // namespace

void ControllerSystem::update() {
  const uint8_t *keys = SDL_GetKeyboardState(nullptr);

  typed.swap(receivedKeys);
  receivedKeys.clear();
  mouseButtonDown =
      (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
  deleteHeld = keys[SDL_SCANCODE_DELETE] != 0;

  const bool letters = keyMode == KeyMode::Game;
  const bool up = keys[SDL_SCANCODE_UP] || (letters && keys[SDL_SCANCODE_W]);
  const bool down =
      keys[SDL_SCANCODE_DOWN] || (letters && keys[SDL_SCANCODE_S]);
  const bool left =
      keys[SDL_SCANCODE_LEFT] || (letters && keys[SDL_SCANCODE_A]);
  const bool right =
      keys[SDL_SCANCODE_RIGHT] || (letters && keys[SDL_SCANCODE_D]);
  states.up = up && !down;
  states.down = down && !up;
  states.left = left && !right;
  states.right = right && !left;
  states.button = keyMode != KeyMode::NameEntry && keys[SDL_SCANCODE_SPACE];

  if (states.button && !states.up && !states.down && !states.left &&
      !states.right) {
    fireLatched = true;
  }

  updateFunctionKey(keys);
}

void ControllerSystem::receiveKey(const SDL_KeyboardEvent &key) {
  const auto scancode = static_cast<std::size_t>(key.keysym.scancode);
  if (scancode >= typingKeys.size()) {
    return;
  }
  if (key.type == SDL_KEYUP) {
    typingKeys[scancode] = false;
    return;
  }
  const std::optional<char> character = typedCharacter(key.keysym.sym);
  if (key.repeat == 0) {
    typingKeys[scancode] = character.has_value();
  }
  if (character && typingKeys[scancode]) {
    receivedKeys += *character;
  }
}

void ControllerSystem::setKeyMode(KeyMode mode) { keyMode = mode; }

void ControllerSystem::clearFireLatch() { fireLatched = false; }

bool ControllerSystem::isFireLatched() const { return fireLatched; }

bool ControllerSystem::isMouseButtonDown() const { return mouseButtonDown; }

bool ControllerSystem::isDeleteHeld() const { return deleteHeld; }

const std::string &ControllerSystem::typedKeys() const { return typed; }

std::optional<char>
ControllerSystem::typedCharacter(SDL_Keycode keycode) const {
  if (keyMode == KeyMode::Game) {
    return std::nullopt;
  }
  switch (keycode) {
  case SDLK_BACKSPACE:
    return '\b';
  case SDLK_RETURN:
  case SDLK_KP_ENTER:
    return '\r';
  case SDLK_SPACE:
    if (keyMode == KeyMode::NameEntry) {
      return ' ';
    }
    return std::nullopt;
  default:
    break;
  }
  if (keycode >= FIRST_PRINTABLE && keycode <= LAST_PRINTABLE) {
    return static_cast<char>(keycode);
  }
  return std::nullopt;
}

std::optional<FunctionKey> ControllerSystem::functionKey() const {
  return pressedFunctionKey;
}

int16_t ControllerSystem::joystick() const {
  return static_cast<int16_t>((states.up ? 1 : 0) | (states.down ? 2 : 0) |
                              (states.left ? 4 : 0) | (states.right ? 8 : 0) |
                              (states.button ? 16 : 0));
}

void ControllerSystem::updateFunctionKey(const uint8_t *keys) {
  constexpr std::array<std::pair<SDL_Scancode, FunctionKey>, 5> KEYS = {{
      {SDL_SCANCODE_F1, FunctionKey::F1},
      {SDL_SCANCODE_F2, FunctionKey::F2},
      {SDL_SCANCODE_F3, FunctionKey::F3},
      {SDL_SCANCODE_F4, FunctionKey::F4},
      {SDL_SCANCODE_ESCAPE, FunctionKey::Escape},
  }};
  pressedFunctionKey.reset();
  for (std::size_t i = 0; i < KEYS.size(); ++i) {
    const bool down = keys[KEYS[i].first];
    if (down && !functionKeysDown[i] && !pressedFunctionKey) {
      pressedFunctionKey = KEYS[i].second;
    }
    functionKeysDown[i] = down;
  }
}

} // namespace openfranko::src::systems
