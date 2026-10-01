#ifndef SYSTEMS_GRAPHICS_PIXELOPS_H_
#define SYSTEMS_GRAPHICS_PIXELOPS_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {
namespace pixels {

struct Source {
  const uint8_t *pixels = nullptr;
  int pitch = 0;
};

struct Target {
  uint8_t *pixels = nullptr;
  int pitch = 0;
};

void copy(Source source, Target target, int width, int height);
void move(Source source, Target target, int width, int height);
void draw(Source source, Target target, int width, int height, bool transparent,
          bool mirrored);
void fill(Target target, int width, int height, uint8_t value);
void finish();

} // namespace pixels
} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_PIXELOPS_H_
