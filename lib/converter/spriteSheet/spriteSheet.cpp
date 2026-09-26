#include "spriteSheet.h"
#include "../../bmpWriter/bmpWriter.h"
#include "../../helpers/helpers.h"
#include "../amosCompact/Consts.h"
#include "../shared/decodeImage.h"
#include "../shared/headers.h"
#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace openfranko::lib::converter::spriteSheet {

using converter::decodeAmosBitmap;
using converter::DecodedImage;

static constexpr size_t BANK_HEADER_SIZE = 12;
static constexpr size_t DESCRIPTOR_SIZE = 10;
static constexpr size_t BMP_HOTSPOT_X_OFFSET = 6;
static constexpr size_t BMP_HOTSPOT_Y_OFFSET = 8;
static constexpr size_t BMP_PALETTE_OFFSET = 54;

static constexpr int FONT_FIRST_SPRITE = 43;
static constexpr int FONT_LAST_SPRITE = 100;

static constexpr int LOGO_REFLECTION_FIRST_SPRITE = 4;
static constexpr int LOGO_REFLECTION_LAST_SPRITE = 9;

static void setBmpPaletteEntry(std::vector<uint8_t> &bmp, int index, uint8_t r,
                               uint8_t g, uint8_t b) {
  size_t offset = BMP_PALETTE_OFFSET + static_cast<size_t>(index) * 4;
  if (offset + 4 > bmp.size()) {
    return;
  }
  bmp[offset + 0] = b;
  bmp[offset + 1] = g;
  bmp[offset + 2] = r;
  bmp[offset + 3] = 0;
}

void embedBmpHotspot(std::vector<uint8_t> &bmp, uint16_t x, uint16_t y) {
  if (bmp.size() < 10 || bmp[0] != 'B' || bmp[1] != 'M') {
    return;
  }

  bmp[BMP_HOTSPOT_X_OFFSET + 0] = static_cast<uint8_t>(x & 0xFF);
  bmp[BMP_HOTSPOT_X_OFFSET + 1] = static_cast<uint8_t>((x >> 8) & 0xFF);
  bmp[BMP_HOTSPOT_Y_OFFSET + 0] = static_cast<uint8_t>(y & 0xFF);
  bmp[BMP_HOTSPOT_Y_OFFSET + 1] = static_cast<uint8_t>((y >> 8) & 0xFF);
}

SpriteBankHeader parseHeader(const std::vector<uint8_t> &data) {
  if (data.size() < BANK_HEADER_SIZE) {
    throw std::runtime_error("Data too small for sprite bank header");
  }

  SpriteBankHeader header;
  helpers::BigEndianReader reader(data);
  header.count = reader.readUint16(0);
  header.maxWidth = reader.readUint16(2);
  header.maxHeight = reader.readUint16(4);
  header.numColors = reader.readUint16(6);
  header.samBankOffset = reader.readUint32(8);

  if (header.count == 0 || header.count > 200) {
    throw std::runtime_error("Invalid sprite count: " +
                             std::to_string(header.count));
  }

  size_t tableEnd = BANK_HEADER_SIZE + header.count * DESCRIPTOR_SIZE;
  if (data.size() < tableEnd) {
    throw std::runtime_error("Data too small for descriptor table");
  }

  header.descriptors.resize(header.count);
  for (uint16_t i = 0; i < header.count; i++) {
    size_t off = BANK_HEADER_SIZE + i * DESCRIPTOR_SIZE;
    header.descriptors[i].wordOffset = reader.readUint16(off + 0);
    header.descriptors[i].widthWords = reader.readUint16(off + 2);
    header.descriptors[i].height = reader.readUint16(off + 4);
    header.descriptors[i].hotspotX = reader.readUint16(off + 6);
    header.descriptors[i].hotspotY = reader.readUint16(off + 8);
  }

  return header;
}

SpriteSheet convertToSheet(const std::vector<uint8_t> &data,
                           const std::vector<uint16_t> &palette, int columns) {
  if (columns <= 0) {
    throw std::runtime_error("Column count must be positive");
  }

  auto header = parseHeader(data);

  std::vector<DecodedImage> sprites;
  sprites.reserve(header.count);
  std::vector<std::string> spriteErrors(header.count);

  uint16_t maxW = 0;
  uint16_t maxH = 0;
  int okCount = 0;

  for (uint16_t i = 0; i < header.count; i++) {
    size_t bmPos = BANK_HEADER_SIZE +
                   static_cast<size_t>(header.descriptors[i].wordOffset) * 2;

    try {
      auto img = decodeAmosBitmap(data, bmPos, palette.data(),
                                  static_cast<int>(palette.size()));
      if (!img.pixels.empty()) {
        if (img.width > maxW) {
          maxW = img.width;
        }
        if (img.height > maxH) {
          maxH = img.height;
        }
        okCount++;
      } else {
        spriteErrors[i] = "Sprite decoded to an empty image";
      }
      sprites.push_back(std::move(img));
    } catch (const std::exception &e) {
      spriteErrors[i] = e.what();
      sprites.push_back({});
    }
  }

  if (okCount == 0) {
    throw std::runtime_error("No valid sprites found in bank");
  }

  int rows = (header.count + columns - 1) / columns;
  uint32_t cellW = maxW + 2;
  uint32_t cellH = maxH + 2;
  uint32_t sheetW = static_cast<uint32_t>(columns) * cellW;
  uint32_t sheetH = static_cast<uint32_t>(rows) * cellH;

  std::vector<uint8_t> sheet(sheetW * sheetH, 0);

  for (int i = 0; i < static_cast<int>(sprites.size()); i++) {
    const auto &spr = sprites[i];
    if (spr.pixels.empty()) {
      continue;
    }

    int col = i % columns;
    int row = i / columns;
    uint32_t ox = static_cast<uint32_t>(col) * cellW + 1;
    uint32_t oy = static_cast<uint32_t>(row) * cellH + 1;

    for (uint32_t y = 0; y < spr.height; y++) {
      for (uint32_t x = 0; x < spr.width; x++) {
        sheet[(oy + y) * sheetW + (ox + x)] = spr.pixels[y * spr.width + x];
      }
    }
  }

  return {bmpWriter::pixelsToBmp(sheetW, sheetH, sheet.data(), palette.data(),
                                 static_cast<int>(palette.size())),
          std::move(spriteErrors)};
}

std::vector<ConvertedSprite>
convertToIndividual(const std::vector<uint8_t> &data,
                    const std::vector<uint16_t> &palette) {
  auto header = parseHeader(data);

  std::vector<ConvertedSprite> results;
  results.reserve(header.count);

  for (uint16_t i = 0; i < header.count; i++) {
    const auto &descriptor = header.descriptors[i];
    size_t bmPos =
        BANK_HEADER_SIZE + static_cast<size_t>(descriptor.wordOffset) * 2;

    try {
      auto img = decodeAmosBitmap(data, bmPos, palette.data(),
                                  static_cast<int>(palette.size()));
      if (!img.pixels.empty()) {
        auto bmp = bmpWriter::pixelsToBmp(img.width, img.height,
                                          img.pixels.data(), palette.data(),
                                          static_cast<int>(palette.size()));
        embedBmpHotspot(bmp, descriptor.hotspotX, descriptor.hotspotY);
        results.push_back({std::move(bmp), {}});
      } else {
        results.push_back({{}, "Sprite decoded to an empty image"});
      }
    } catch (const std::exception &e) {
      results.push_back({{}, e.what()});
    }
  }

  return results;
}

void applySpritePaletteFixes(const std::string &fileId,
                             std::vector<ConvertedSprite> &sprites) {
  if (gameData::version10Id(fileId) != gameData::fileIds::SUNSET_PALETTE &&
      fileId != gameData::version12::fileIds::WORLD_SOFTWARE_PALETTE) {
    return;
  }
  for (int i = FONT_FIRST_SPRITE;
       i <= FONT_LAST_SPRITE && i < static_cast<int>(sprites.size()); i++) {
    setBmpPaletteEntry(sprites[i].bmpData, 1, 0xFF, 0xFF, 0xFF);
    setBmpPaletteEntry(sprites[i].bmpData, 2, 0xAA, 0xAA, 0xAA);
  }
}

std::string_view paletteScreen(const std::string &fileId) {
  return fileId == gameData::version12::fileIds::WORLD_SOFTWARE_PALETTE
             ? gameData::version12::fileIds::WORLD_SOFTWARE_LOGO
             : std::string_view();
}

void applyScreenPalette(const std::string &fileId,
                        const std::vector<uint8_t> &screen,
                        std::vector<ConvertedSprite> &sprites) {
  if (paletteScreen(fileId).empty()) {
    return;
  }
  if (screen.size() < amosCompact::consts::SPACK_HEADER_SIZE ||
      helpers::BigEndianReader(screen).readUint32(0) !=
          amosCompact::consts::SPACK_SCREEN_HEADER) {
    throw std::runtime_error("Not a packed screen");
  }
  const auto header = headers::parseSPACKHeader(screen);
  const int colours =
      std::min<int>(header.numberOfColors,
                    static_cast<int>(amosCompact::consts::SPACK_PALETTE_SIZE));
  for (int i = LOGO_REFLECTION_FIRST_SPRITE;
       i <= LOGO_REFLECTION_LAST_SPRITE && i < static_cast<int>(sprites.size());
       i++) {
    for (int colour = 0; colour < colours; colour++) {
      const uint16_t amiga = header.amigaPalette[colour];
      setBmpPaletteEntry(sprites[i].bmpData, colour,
                         static_cast<uint8_t>(((amiga >> 8) & 0xF) * 17),
                         static_cast<uint8_t>(((amiga >> 4) & 0xF) * 17),
                         static_cast<uint8_t>((amiga & 0xF) * 17));
    }
  }
}

std::vector<uint16_t> selectPalette(const std::string &fileId) {
  return palettes::selectPalette(fileId);
}

} // namespace openfranko::lib::converter::spriteSheet
