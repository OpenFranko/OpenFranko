#ifndef SPRITESHEET_H_
#define SPRITESHEET_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace spriteSheet {

struct SpriteDescriptor {
  uint16_t wordOffset = 0;
  uint16_t widthWords = 0;
  uint16_t height = 0;
  uint16_t hotspotX = 0;
  uint16_t hotspotY = 0;
};

struct SpriteBankHeader {
  uint16_t count = 0;
  uint16_t maxWidth = 0;
  uint16_t maxHeight = 0;
  uint16_t numberOfColors = 0;
  uint32_t samBankOffset = 0;
  std::vector<SpriteDescriptor> descriptors;
};

struct ConvertedSprite {
  std::vector<uint8_t> data;
  std::string error;
};

struct SpriteSheet {
  std::vector<uint8_t> data;
  std::vector<std::string> spriteErrors;
};

SpriteBankHeader parseHeader(const std::vector<uint8_t> &data);

SpriteSheet convertToSheet(const std::vector<uint8_t> &data,
                           const std::vector<uint16_t> &palette,
                           int columns = 5);

std::vector<ConvertedSprite>
convertToIndividual(const std::vector<uint8_t> &data,
                    const std::vector<uint16_t> &palette);

void applySpritePaletteFixes(const std::string &fileId,
                             std::vector<ConvertedSprite> &sprites);

std::string_view paletteScreen(const std::string &fileId);

void applyScreenPalette(const std::string &fileId,
                        const std::vector<uint8_t> &screen,
                        std::vector<ConvertedSprite> &sprites);

} // namespace spriteSheet
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // SPRITESHEET_H_
