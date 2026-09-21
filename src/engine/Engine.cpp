#include "Engine.h"

namespace openfranko::src::engine {

Engine::Engine() : running(true) {
  videoSystem.createScreen(0, 320, 240);
  videoSystem.switchScreen(0);

  videoSystem.loadImage("background", "assets/0388.bmp");

  audioSystem.loadMusic("assets/0261.s3m");
  audioSystem.playMusic();
}

Engine::~Engine() { SDL_Quit(); }

bool Engine::isRunning() {
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT)
      running = false;
  }
  return running;
}

void Engine::update() {
  controllerSystem.update();
  videoSystem.drawImage("background", 0, 0);
  videoSystem.sync();
}

} // namespace openfranko::src::engine
