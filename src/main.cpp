#include "engine/Engine.h"

using namespace openfranko::src;

int main() {

  engine::Engine engine;

  const double ticksPerSecond =
      static_cast<double>(SDL_GetPerformanceFrequency());
  double nextFrame = static_cast<double>(SDL_GetPerformanceCounter());

  while (engine.isRunning()) {
    engine.update();

    const double frameTicks = ticksPerSecond / engine.refreshRate();
    nextFrame += frameTicks;
    const double now = static_cast<double>(SDL_GetPerformanceCounter());
    if (nextFrame > now) {
      SDL_Delay(
          static_cast<uint32_t>((nextFrame - now) * 1000.0 / ticksPerSecond));
    } else if (now - nextFrame > frameTicks) {
      nextFrame = now;
    }
  }
  return 0;
}
