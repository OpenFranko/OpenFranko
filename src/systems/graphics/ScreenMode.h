#ifndef SYSTEMS_GRAPHICS_SCREENMODE_H_
#define SYSTEMS_GRAPHICS_SCREENMODE_H_

#include <cstddef>
#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {

struct ScreenMode {
  int width = 0;
  int height = 0;
  int hertz = 0;
};

bool isRefreshedAt(const ScreenMode &mode, int hertz);

std::optional<std::size_t> fullscreenMode(const std::vector<ScreenMode> &modes,
                                          const ScreenMode &desktop, int hertz);

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_SCREENMODE_H_
