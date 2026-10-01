#ifndef SYSTEMS_JAGUAR_VIDEO_H_
#define SYSTEMS_JAGUAR_VIDEO_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {

inline constexpr int PIXEL_CLOCKS = 4;

struct Geometry {
  bool ntsc = false;
  int hertz = 50;
  int columns = 0;
  int rows = 0;
  int firstHalfLine = 0;
  int lastHalfLine = 0;
};

Geometry detectGeometry();
void setupVideo(const Geometry &geometry);
void setOlp(uint32_t address);
bool isBlanking(const Geometry &geometry);
void waitBlanking(const Geometry &geometry);
void waitDisplay(const Geometry &geometry);
inline uint16_t toRgb16(uint16_t amigaColor) {
  const uint16_t red = (amigaColor >> 8) & 0xF;
  const uint16_t green = (amigaColor >> 4) & 0xF;
  const uint16_t blue = amigaColor & 0xF;
  return static_cast<uint16_t>((red << 1 | red >> 3) << 11 |
                               (blue << 1 | blue >> 3) << 6 |
                               (green << 2 | green >> 2));
}

} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_VIDEO_H_
