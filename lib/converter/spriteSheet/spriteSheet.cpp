#include "spriteSheet.h"
#include "../../bmpWriter/bmpWriter.h"
#include "../../helpers/helpers.h"
#include "../shared/decodeImage.h"
#include <stdexcept>

namespace openfranko::lib::converter::spriteSheet {

using converter::decodeAmosBitmap;
using converter::DecodedImage;

static constexpr size_t BANK_HEADER_SIZE = 12;
static constexpr size_t DESCRIPTOR_SIZE = 10;
static constexpr size_t BMP_HOTSPOT_X_OFFSET = 6;
static constexpr size_t BMP_HOTSPOT_Y_OFFSET = 8;

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

std::vector<uint16_t> selectPalette(const std::string &fileId) {
  return palettes::selectPalette(fileId);
}

} // namespace openfranko::lib::converter::spriteSheet
