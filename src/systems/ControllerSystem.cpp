#include "ControllerSystem.h"
#include <SDL2/SDL.h>
#include <cstddef>
#include <utility>

namespace openfranko::src::systems {

void ControllerSystem::update() {
  const uint8_t *keys = SDL_GetKeyboardState(nullptr);

  clearStates();

  if (keys[SDL_SCANCODE_W] && !keys[SDL_SCANCODE_S]) {
    states.up = true;
  }
  if (keys[SDL_SCANCODE_S] && !keys[SDL_SCANCODE_W]) {
    states.down = true;
  }
  if (keys[SDL_SCANCODE_A] && !keys[SDL_SCANCODE_D]) {
    states.left = true;
  }
  if (keys[SDL_SCANCODE_D] && !keys[SDL_SCANCODE_A]) {
    states.right = true;
  }
  if (keys[SDL_SCANCODE_SPACE]) {
    states.button = true;
  }

  if (states.button && !states.up && !states.down && !states.left &&
      !states.right) {
    fireLatched = true;
  }

  updateTypedLetter(keys);
  updateTypedKey(keys);
  updateFunctionKey(keys);
}

void ControllerSystem::clearFireLatch() { fireLatched = false; }

bool ControllerSystem::isFireLatched() const { return fireLatched; }

std::optional<char> ControllerSystem::typedLetter() const { return letter; }

void ControllerSystem::updateTypedLetter(const uint8_t *keys) {
  letter.reset();
  for (std::size_t i = 0; i < lettersDown.size(); ++i) {
    const SDL_Scancode scancode =
        SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(SDLK_a + i));
    const bool down = keys[scancode];
    if (down && !lettersDown[i] && !letter) {
      letter = static_cast<char>('A' + i);
    }
    lettersDown[i] = down;
  }
}

std::optional<char> ControllerSystem::typedKey() const { return key; }

void ControllerSystem::updateTypedKey(const uint8_t *keys) {
  constexpr std::array<std::pair<SDL_Scancode, char>, 4> KEYS = {{
      {SDL_SCANCODE_SPACE, ' '},
      {SDL_SCANCODE_BACKSPACE, '\b'},
      {SDL_SCANCODE_RETURN, '\r'},
      {SDL_SCANCODE_KP_ENTER, '\r'},
  }};
  key = letter;
  for (std::size_t i = 0; i < KEYS.size(); ++i) {
    const bool down = keys[KEYS[i].first];
    if (down && !editingKeysDown[i] && !key) {
      key = KEYS[i].second;
    }
    editingKeysDown[i] = down;
  }
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

void ControllerSystem::clearStates() {
  states.left = false;
  states.right = false;
  states.up = false;
  states.down = false;
  states.button = false;
}

} // namespace openfranko::src::systems