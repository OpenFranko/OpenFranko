#include "Engine.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>

namespace openfranko::src::engine {

Engine::Engine() : player{videoSystem, controllerSystem}, running(true) {
  videoSystem.createScreen(0, 320, 240);
  videoSystem.switchScreen(0);

  videoSystem.loadBackground("assets/0388.bmp");

  audioSystem.loadMusic("assets/0259.s3m");
  audioSystem.loadSFX("ready", "assets/00FF/00FF_sam12_6573Hz.wav");

  audioSystem.playMusic();
  audioSystem.playSFX("ready");
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
  player.update();
  videoSystem.drawBackground();
  player.draw();
  videoSystem.sync();
}

} // namespace openfranko::src::engine
