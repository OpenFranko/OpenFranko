#ifndef AMOSCOMPACT_DECODEIMAGE_H_
#define AMOSCOMPACT_DECODEIMAGE_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace amosCompact {

struct DecodedImage {
  uint16_t width = 0;
  uint16_t height = 0;
  std::vector<uint8_t> pixels;
};

DecodedImage decodeAmosBitmap(const std::vector<uint8_t> &data, size_t offset,
                              const uint16_t *palette, int numberOfColors);

} // namespace amosCompact
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // AMOSCOMPACT_DECODEIMAGE_H_
