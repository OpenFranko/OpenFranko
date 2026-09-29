#include "spriteSheet.h"
#include "../../binary/binary.h"
#include "../../bmpWriter/bmpWriter.h"
#include "../amosCompact/decodeAmosBitmap.h"
#include "../gameData/gameData.h"
#include "../headers/headers.h"

#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace openfranko::lib::converter::spriteSheet {

namespace {

constexpr size_t BANK_HEADER_SIZE = 12;
constexpr size_t BANK_MAX_WIDTH_OFFSET = 2;
constexpr size_t BANK_MAX_HEIGHT_OFFSET = 4;
constexpr size_t BANK_COLORS_OFFSET = 6;
constexpr size_t BANK_SAM_BANK_POINTER_OFFSET = 8;
constexpr uint16_t MAX_SPRITE_COUNT = 200;

constexpr size_t DESCRIPTOR_SIZE = 10;
constexpr size_t DESCRIPTOR_WIDTH_WORDS_OFFSET = 2;
constexpr size_t DESCRIPTOR_HEIGHT_OFFSET = 4;
constexpr size_t DESCRIPTOR_HOTSPOT_X_OFFSET = 6;
constexpr size_t DESCRIPTOR_HOTSPOT_Y_OFFSET = 8;

constexpr size_t BMP_HOTSPOT_X_OFFSET = 6;
constexpr size_t BMP_HOTSPOT_Y_OFFSET = 8;
constexpr size_t BMP_HOTSPOT_SIZE = 2;
constexpr size_t BMP_PALETTE_OFFSET = 54;
constexpr size_t BMP_PALETTE_ENTRY_SIZE = 4;

constexpr int FONT_FIRST_SPRITE = 43;
constexpr int FONT_LAST_SPRITE = 100;

constexpr int LOGO_REFLECTION_FIRST_SPRITE = 4;
constexpr int LOGO_REFLECTION_LAST_SPRITE = 9;

void setBmpPaletteEntry(std::vector<uint8_t> &bmp, int index, uint8_t r,
                        uint8_t g, uint8_t b) {
  size_t offset =
      BMP_PALETTE_OFFSET + static_cast<size_t>(index) * BMP_PALETTE_ENTRY_SIZE;
  if (offset + BMP_PALETTE_ENTRY_SIZE > bmp.size()) {
    return;
  }
  bmp[offset + 0] = b;
  bmp[offset + 1] = g;
  bmp[offset + 2] = r;
  bmp[offset + 3] = 0;
}

void embedBmpHotspot(std::vector<uint8_t> &bmp, uint16_t x, uint16_t y) {
  if (bmp.size() < BMP_HOTSPOT_Y_OFFSET + BMP_HOTSPOT_SIZE || bmp[0] != 'B' ||
      bmp[1] != 'M') {
    return;
  }

  binary::writeLittleEndian16(bmp, BMP_HOTSPOT_X_OFFSET, x);
  binary::writeLittleEndian16(bmp, BMP_HOTSPOT_Y_OFFSET, y);
}

} // namespace

SpriteBankHeader parseHeader(const std::vector<uint8_t> &data) {
  if (data.size() < BANK_HEADER_SIZE) {
    throw std::runtime_error("Data too small for sprite bank header");
  }

  SpriteBankHeader header;
  binary::BigEndianReader reader(data);
  header.count = reader.readUint16(0);
  header.maxWidth = reader.readUint16(BANK_MAX_WIDTH_OFFSET);
  header.maxHeight = reader.readUint16(BANK_MAX_HEIGHT_OFFSET);
  header.numberOfColors = reader.readUint16(BANK_COLORS_OFFSET);
  header.samBankOffset = reader.readUint32(BANK_SAM_BANK_POINTER_OFFSET);

  if (header.count == 0 || header.count > MAX_SPRITE_COUNT) {
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
    header.descriptors[i].wordOffset = reader.readUint16(off);
    header.descriptors[i].widthWords =
        reader.readUint16(off + DESCRIPTOR_WIDTH_WORDS_OFFSET);
    header.descriptors[i].height =
        reader.readUint16(off + DESCRIPTOR_HEIGHT_OFFSET);
    header.descriptors[i].hotspotX =
        reader.readUint16(off + DESCRIPTOR_HOTSPOT_X_OFFSET);
    header.descriptors[i].hotspotY =
        reader.readUint16(off + DESCRIPTOR_HOTSPOT_Y_OFFSET);
  }

  return header;
}

SpriteSheet convertToSheet(const std::vector<uint8_t> &data,
                           const std::vector<uint16_t> &palette, int columns) {
  if (columns <= 0) {
    throw std::runtime_error("Column count must be positive");
  }

  auto header = parseHeader(data);

  std::vector<amosCompact::DecodedImage> sprites;
  sprites.reserve(header.count);
  std::vector<std::string> spriteErrors(header.count);

  uint16_t maxWidth = 0;
  uint16_t maxHeight = 0;
  int numberOfDecoded = 0;

  for (uint16_t i = 0; i < header.count; i++) {
    size_t bitmapPos =
        BANK_HEADER_SIZE +
        static_cast<size_t>(header.descriptors[i].wordOffset) * 2;

    try {
      auto image = amosCompact::decodeAmosBitmap(data, bitmapPos);
      if (!image.pixels.empty()) {
        if (image.width > maxWidth) {
          maxWidth = image.width;
        }
        if (image.height > maxHeight) {
          maxHeight = image.height;
        }
        numberOfDecoded++;
      } else {
        spriteErrors[i] = "Sprite decoded to an empty image";
      }
      sprites.push_back(std::move(image));
    } catch (const std::exception &e) {
      spriteErrors[i] = e.what();
      sprites.push_back({});
    }
  }

  if (numberOfDecoded == 0) {
    throw std::runtime_error("No valid sprites found in bank");
  }

  int rows = (header.count + columns - 1) / columns;
  uint32_t cellWidth = maxWidth + 2;
  uint32_t cellHeight = maxHeight + 2;
  uint32_t sheetWidth = static_cast<uint32_t>(columns) * cellWidth;
  uint32_t sheetHeight = static_cast<uint32_t>(rows) * cellHeight;

  std::vector<uint8_t> sheet(sheetWidth * sheetHeight, 0);

  for (int i = 0; i < static_cast<int>(sprites.size()); i++) {
    const auto &sprite = sprites[i];
    if (sprite.pixels.empty()) {
      continue;
    }

    int column = i % columns;
    int row = i / columns;
    uint32_t cellX = static_cast<uint32_t>(column) * cellWidth + 1;
    uint32_t cellY = static_cast<uint32_t>(row) * cellHeight + 1;

    for (uint32_t y = 0; y < sprite.height; y++) {
      for (uint32_t x = 0; x < sprite.width; x++) {
        sheet[(cellY + y) * sheetWidth + (cellX + x)] =
            sprite.pixels[y * sprite.width + x];
      }
    }
  }

  return {bmpWriter::pixelsToBmp(sheetWidth, sheetHeight, sheet.data(),
                                 palette.data(),
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
    size_t bitmapPos =
        BANK_HEADER_SIZE + static_cast<size_t>(descriptor.wordOffset) * 2;

    try {
      auto image = amosCompact::decodeAmosBitmap(data, bitmapPos);
      if (!image.pixels.empty()) {
        auto bmp = bmpWriter::pixelsToBmp(image.width, image.height,
                                          image.pixels.data(), palette.data(),
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
    setBmpPaletteEntry(sprites[i].data, 1, 0xFF, 0xFF, 0xFF);
    setBmpPaletteEntry(sprites[i].data, 2, 0xAA, 0xAA, 0xAA);
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
  if (screen.size() < headers::SPACK_HEADER_SIZE ||
      binary::BigEndianReader(screen).readUint32(0) !=
          headers::SPACK_SCREEN_HEADER) {
    throw std::runtime_error("Not a packed screen");
  }
  const auto header = headers::parseSpackHeader(screen);
  const int colors = std::min<int>(
      header.numberOfColors, static_cast<int>(headers::SPACK_COLOR_COUNT));
  for (int i = LOGO_REFLECTION_FIRST_SPRITE;
       i <= LOGO_REFLECTION_LAST_SPRITE && i < static_cast<int>(sprites.size());
       i++) {
    for (int color = 0; color < colors; color++) {
      const uint16_t amiga = header.amigaPalette[color];
      setBmpPaletteEntry(sprites[i].data, color,
                         static_cast<uint8_t>(((amiga >> 8) & 0xF) * 17),
                         static_cast<uint8_t>(((amiga >> 4) & 0xF) * 17),
                         static_cast<uint8_t>((amiga & 0xF) * 17));
    }
  }
}

} // namespace openfranko::lib::converter::spriteSheet
