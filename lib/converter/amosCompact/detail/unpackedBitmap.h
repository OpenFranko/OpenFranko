#ifndef UNPACKEDBITMAP_H_
#define UNPACKEDBITMAP_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace amosCompact {
namespace detail {

struct UnpackedBitmap {
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t numberOfBitplanes = 0;
  uint16_t palette[32] = {};
  std::vector<std::vector<uint8_t>> bitplaneData;
  size_t bytesPerPlane = 0;
  std::vector<uint8_t> chunkyPixels;
};

} // namespace detail
} // namespace amosCompact
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // UNPACKEDBITMAP_H_
