#ifndef DECODEIMAGE_H_
#define DECODEIMAGE_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {

struct DecodedImage {
  uint16_t width = 0;
  uint16_t height = 0;
  std::vector<uint8_t> pixels;
};

DecodedImage decodeAmosBitmap(const std::vector<uint8_t> &data, size_t offset,
                              const uint16_t *palette, int numberOfColors);

} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // DECODEIMAGE_H_
