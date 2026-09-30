#include "ScreenMode.h"

#include <cstdlib>

namespace openfranko::src::systems::graphics {
namespace {

constexpr int HERTZ_TOLERANCE = 1;

long area(const ScreenMode &mode) {
  return static_cast<long>(mode.width) * mode.height;
}

} // namespace

bool isRefreshedAt(const ScreenMode &mode, int hertz) {
  return std::abs(mode.hertz - hertz) <= HERTZ_TOLERANCE;
}

std::optional<std::size_t> fullscreenMode(const std::vector<ScreenMode> &modes,
                                          const ScreenMode &desktop,
                                          int hertz) {
  if (isRefreshedAt(desktop, hertz)) {
    return std::nullopt;
  }
  std::optional<std::size_t> chosen;
  for (std::size_t i = 0; i < modes.size(); ++i) {
    const ScreenMode &mode = modes[i];
    if (!isRefreshedAt(mode, hertz)) {
      continue;
    }
    if (mode.width == desktop.width && mode.height == desktop.height) {
      return i;
    }
    if (!chosen || area(mode) > area(modes[*chosen])) {
      chosen = i;
    }
  }
  return chosen;
}

} // namespace openfranko::src::systems::graphics
