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

struct ConvertedSprite {
  std::vector<uint8_t> bmpData;
  std::string error;
};

struct SpriteSheet {
  std::vector<uint8_t> bmpData;
  std::vector<std::string> spriteErrors;
};

SpriteBankHeader parseHeader(const std::vector<uint8_t> &data);

SpriteSheet convertToSheet(const std::vector<uint8_t> &decompressedData,
                           const std::vector<uint16_t> &palette,
                           int columns = 5);

std::vector<ConvertedSprite>
convertToIndividual(const std::vector<uint8_t> &decompressedData,
                    const std::vector<uint16_t> &palette);

void applySpritePaletteFixes(const std::string &fileId,
                             std::vector<ConvertedSprite> &sprites);

std::vector<uint16_t> selectPalette(const std::string &fileId);

} // namespace spriteSheet
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // SPRITESHEET_H_
