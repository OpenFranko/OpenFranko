#ifndef UNPACKEDBITMAP_H_
#define UNPACKEDBITMAP_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {
namespace detail {

struct UnpackedBitmap {
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t numberOfBitplanes = 0;
  uint16_t palette[32] = {};
  uint8_t *bitplaneData[6] = {};
  size_t bytesPerPlane = 0;
  uint8_t *chunkyPixels = nullptr;
};

} // namespace detail
} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko

#endif // UNPACKEDBITMAP_H_