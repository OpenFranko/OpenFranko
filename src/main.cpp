#include "engine/Engine.h"

using namespace openfranko::src;

int main() {

  engine::Engine engine;

  const int FPS = 50;
  const int frameDelay = 1000 / FPS;

  uint32_t frameStart;
  int frameTime;

  while (engine.isRunning()) {
    frameStart = SDL_GetTicks();

    engine.update();

    frameTime = SDL_GetTicks() - frameStart;

    if (frameDelay > frameTime) {
      SDL_Delay(frameDelay - frameTime);
    }
  }
  return 0;
}
