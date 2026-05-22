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

Engine::Engine() : running(true) {}

Engine::~Engine() { SDL_Quit(); }

bool Engine::isRunning() {
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT)
      running = false;
  }
  return running;
}

} // namespace openfranko::src::engine
