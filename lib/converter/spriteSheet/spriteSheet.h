#ifndef SPRITESHEET_H_
#define SPRITESHEET_H_

#include "Palettes.h"
#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace spriteSheet {

struct SpriteDescriptor {
  uint16_t wordOffset;
  uint16_t widthWords;
  uint16_t height;
  uint16_t hotspotX;
  uint16_t hotspotY;
};

struct SpriteBankHeader {
  uint16_t count;
  uint16_t maxWidth;
  uint16_t maxHeight;
  uint16_t numColors;
  uint32_t samBankOffset;
  std::vector<SpriteDescriptor> descriptors;
};

struct SpriteBitmap {
  std::vector<uint8_t> bmpData;
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t hotspotX = 0;
  uint16_t hotspotY = 0;
};

SpriteBankHeader parseHeader(const std::vector<uint8_t> &data);

std::vector<uint8_t> convertToSheet(const std::vector<uint8_t> &decompressedData,
                                    const std::vector<uint16_t> &palette,
                                    int columns = 5);

std::vector<std::vector<uint8_t>>
convertToIndividual(const std::vector<uint8_t> &decompressedData,
                    const std::vector<uint16_t> &palette);

std::vector<SpriteBitmap>
convertToIndividualWithHotspots(const std::vector<uint8_t> &decompressedData,
                                const std::vector<uint16_t> &palette);

std::vector<uint16_t> selectPalette(const std::string &fileId);

} // namespace spriteSheet
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // SPRITESHEET_H_
