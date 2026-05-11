#ifndef HEADERS_H_
#define HEADERS_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace headers {

struct SPACKHeader {
  uint16_t screenWidth;
  uint16_t screenHeight;
  uint16_t windowX;
  uint16_t windowY;
  uint16_t windowWidth;
  uint16_t windowHeight;
  uint16_t viewX;
  uint16_t viewY;
  uint16_t displayModeFlags;
  uint16_t numberOfColors;
  uint16_t numberOfBitplanes;
  std::vector<uint16_t> amigaPalette;
};

struct BitmapHeader {
  int16_t xOffset;
  int16_t yOffset;
  uint16_t bytesWidth;
  uint16_t rowsHeight;
  uint16_t tileHeight;
  uint16_t numberOfBitplanes;
  uint32_t offsetToByteTable;
  uint32_t offsetToPointerTable;
};

} // namespace headers
} // namespace decompressor
} // namespace lib
} // namespace openfranko

#endif // HEADERS_H_