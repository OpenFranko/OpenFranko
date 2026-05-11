#ifndef HEADERS_H_
#define HEADERS_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {
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
  uint16_t amigaPalette[32];
};

SPACKHeader parseSPACKHeader(const std::vector<uint8_t> &data);

struct BitmapHeader {
  int16_t xOffset;
  int16_t yOffset;
  uint16_t gridX;
  uint16_t gridY;
  uint16_t tileHeight;
  uint16_t numberOfBitplanes;
  uint32_t offsetToByteTable2;
  uint32_t offsetToPointerBitstream;
};

BitmapHeader parseBitmapHeader(const std::vector<uint8_t> &data);

} // namespace headers
} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko

#endif // HEADERS_H_