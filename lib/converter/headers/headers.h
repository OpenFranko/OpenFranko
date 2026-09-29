#ifndef HEADERS_H_
#define HEADERS_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace headers {

constexpr uint32_t SPACK_SCREEN_HEADER = 0x12031990u;
constexpr uint32_t AMOS_BMCODE = 0x06071963u;

constexpr size_t MAX_SUPPORTED_BITPLANES = 6;

constexpr size_t SPACK_HEADER_SIZE = 90;
constexpr size_t SPACK_PALETTE_SIZE = 32;

constexpr size_t PACKED_BITMAP_HEADER_SIZE = 24;

struct SpackHeader {
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

SpackHeader parseSpackHeader(const std::vector<uint8_t> &data);

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
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // HEADERS_H_
