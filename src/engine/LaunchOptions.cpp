#include "LaunchOptions.h"

#include <stdexcept>
#include <string>

namespace openfranko::src::engine {
namespace {

constexpr auto SHORT_FULLSCREEN = "-f";
constexpr auto LONG_FULLSCREEN = "--fullscreen";
constexpr auto USAGE_OPTIONS = " [-f|--fullscreen]";

} // namespace

LaunchOptions parseLaunchOptions(int argc, const char *const *argv) {
  LaunchOptions options;
  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument == SHORT_FULLSCREEN || argument == LONG_FULLSCREEN) {
      options.fullscreen = true;
    } else {
      throw std::invalid_argument("Unknown option: " + argument +
                                  "\nUsage: " + argv[0] + USAGE_OPTIONS);
    }
  }
  return options;
}

} // namespace openfranko::src::engine
