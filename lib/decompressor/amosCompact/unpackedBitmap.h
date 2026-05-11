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
  std::vector<uint16_t> palette;
  std::vector<uint8_t *> bitplaneData;
  size_t bytesPerPlane;
  std::vector<uint8_t> chunkyPixels;
};

} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko