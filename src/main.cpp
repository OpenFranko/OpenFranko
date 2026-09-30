#include "engine/Engine.h"

#include <exception>
#include <iostream>

using namespace openfranko::src;

int main() {
  try {
    engine::Engine engine;
    engine.run();
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << std::endl;
    return 1;
  }
  return 0;
}
