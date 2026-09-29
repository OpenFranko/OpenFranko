#ifndef ENGINE_EFFECTS_COLOR_AMIGAPALETTE_H_
#define ENGINE_EFFECTS_COLOR_AMIGAPALETTE_H_

#include "../../../systems/graphics/Display.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {
namespace color {

using AmigaColor = uint16_t;
using AmigaPalette = std::vector<AmigaColor>;

inline constexpr AmigaColor BLACK = 0x000;
inline constexpr AmigaColor WHITE = 0xFFF;

struct Rgb {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
};

constexpr Rgb toRgb(AmigaColor color) {
  return {
      static_cast<uint8_t>(((color >> 8) & 0xF) *
                           systems::graphics::CHANNEL_STEP),
      static_cast<uint8_t>(((color >> 4) & 0xF) *
                           systems::graphics::CHANNEL_STEP),
      static_cast<uint8_t>((color & 0xF) * systems::graphics::CHANNEL_STEP)};
}

} // namespace color
} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_COLOR_AMIGAPALETTE_H_
