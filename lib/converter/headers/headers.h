#ifndef HEADERS_H_
#define HEADERS_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace headers {

inline constexpr uint32_t SPACK_SCREEN_HEADER = 0x12031990u;
inline constexpr uint32_t AMOS_BMCODE = 0x06071963u;

inline constexpr std::size_t MAX_SUPPORTED_BITPLANES = 6;

inline constexpr std::size_t SPACK_HEADER_SIZE = 90;
inline constexpr std::size_t SPACK_COLOR_COUNT = 32;

inline constexpr std::size_t PACKED_BITMAP_HEADER_SIZE = 24;

struct SpackHeader {
  uint16_t screenWidth = 0;
  uint16_t screenHeight = 0;
  uint16_t windowX = 0;
  uint16_t windowY = 0;
  uint16_t windowWidth = 0;
  uint16_t windowHeight = 0;
  uint16_t viewX = 0;
  uint16_t viewY = 0;
  uint16_t displayModeFlags = 0;
  uint16_t numberOfColors = 0;
  uint16_t numberOfBitplanes = 0;
  uint16_t amigaPalette[SPACK_COLOR_COUNT] = {};
};

SpackHeader parseSpackHeader(const std::vector<uint8_t> &data);

struct BitmapHeader {
  int16_t xOffset = 0;
  int16_t yOffset = 0;
  uint16_t gridX = 0;
  uint16_t gridY = 0;
  uint16_t tileHeight = 0;
  uint16_t numberOfBitplanes = 0;
  uint32_t offsetToByteTable2 = 0;
  uint32_t offsetToPointerBitstream = 0;
};

BitmapHeader parseBitmapHeader(const std::vector<uint8_t> &data);

} // namespace headers
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // HEADERS_H_
