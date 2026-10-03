#include "engine/Engine.h"

#include <cstdio>
#include <exception>

using namespace openfranko::src;

int main() {
  try {
    engine::Engine engine;
    engine.run();
  } catch (const std::exception &error) {
    std::fprintf(stderr, "Error: %s\n", error.what());
    return 1;
  }
  return 0;
}
