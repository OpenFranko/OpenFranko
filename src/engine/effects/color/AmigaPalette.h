#ifndef ENGINE_EFFECTS_AMIGAPALETTE_H_
#define ENGINE_EFFECTS_AMIGAPALETTE_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

using AmigaColor = uint16_t;
using AmigaPalette = std::vector<AmigaColor>;

struct Rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

constexpr Rgb toRgb(AmigaColor color) {
  return {static_cast<uint8_t>(((color >> 8) & 0xF) * 17),
          static_cast<uint8_t>(((color >> 4) & 0xF) * 17),
          static_cast<uint8_t>((color & 0xF) * 17)};
}

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_AMIGAPALETTE_H_
