#ifndef UNPACKEDBITMAP_H_
#define UNPACKEDBITMAP_H_

#include "Consts.h"
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {

struct UnpackedBitmap {
  uint16_t width;
  uint16_t height;
  uint16_t numberOfBitplanes;
  uint16_t palette[32];
  uint8_t *bitplaneData[consts::MAX_SUPPORTED_BITPLANES];
  size_t bytesPerPlane;
  uint8_t *chunkyPixels;
};

} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko

#endif // UNPACKEDBITMAP_H_