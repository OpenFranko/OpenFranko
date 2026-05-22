#include "engine/Engine.h"

using namespace openfranko::src;

int main() {

  engine::Engine engine;

  while (engine.isRunning()) {
    engine.update();
  }
  return 0;
}
