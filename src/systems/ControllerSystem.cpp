#include "ControllerSystem.h"
#include <SDL2/SDL.h>

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
}

void ControllerSystem::clearFireLatch() { fireLatched = false; }

bool ControllerSystem::isFireLatched() const { return fireLatched; }

void ControllerSystem::clearStates() {
  states.left = false;
  states.right = false;
  states.up = false;
  states.down = false;
  states.button = false;
}

} // namespace openfranko::src::systems