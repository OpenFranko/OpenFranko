#include "engine/Engine.h"
#include "engine/LaunchOptions.h"

#include <cstdio>
#include <exception>

using namespace openfranko::src;

int main(int argc, char **argv) {
  try {
    engine::Engine engine(engine::parseLaunchOptions(argc, argv));
    engine.run();
  } catch (const std::exception &error) {
    std::fprintf(stderr, "Error: %s\n", error.what());
    return 1;
  }
  return 0;
}
