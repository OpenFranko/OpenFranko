#include "ControllerSystem.h"
#include <SDL2/SDL.h>
#include <cstddef>

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

void ControllerSystem::clearStates() {
  states.left = false;
  states.right = false;
  states.up = false;
  states.down = false;
  states.button = false;
}

} // namespace openfranko::src::systems