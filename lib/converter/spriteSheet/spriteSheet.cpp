#include "spriteSheet.h"
#include "../../bmpWriter/bmpWriter.h"
#include "../../decompressor/helpers/helpers.h"
#include "../shared/decodeImage.h"
#include <stdexcept>

namespace openfranko::lib::converter::spriteSheet {

namespace helpers = decompressor::helpers;
using converter::DecodedImage;
using converter::decodeAmosBitmap;

static constexpr size_t BANK_HEADER_SIZE = 12;
static constexpr size_t DESCRIPTOR_SIZE = 10;

SpriteBankHeader parseHeader(const std::vector<uint8_t> &data) {
  if (data.size() < BANK_HEADER_SIZE) {
    throw std::runtime_error("Data too small for sprite bank header");
  }

  SpriteBankHeader header;
  header.count = helpers::readUint16BigEndian(data, 0);
  header.maxWidth = helpers::readUint16BigEndian(data, 2);
  header.maxHeight = helpers::readUint16BigEndian(data, 4);
  header.numColors = helpers::readUint16BigEndian(data, 6);
  header.samBankOffset = helpers::readUint32BigEndian(data, 8);

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
    header.descriptors[i].wordOffset =
        helpers::readUint16BigEndian(data, off + 0);
    header.descriptors[i].widthWords =
        helpers::readUint16BigEndian(data, off + 2);
    header.descriptors[i].height =
        helpers::readUint16BigEndian(data, off + 4);
    header.descriptors[i].hotspotX =
        helpers::readUint16BigEndian(data, off + 6);
    header.descriptors[i].hotspotY =
        helpers::readUint16BigEndian(data, off + 8);
  }

  return header;
}

std::vector<uint8_t> convertToSheet(const std::vector<uint8_t> &data,
                                    const std::vector<uint16_t> &palette,
                                    int columns) {
  auto header = parseHeader(data);

  std::vector<DecodedImage> sprites;
  sprites.reserve(header.count);

  uint16_t maxW = 0;
  uint16_t maxH = 0;
  int okCount = 0;

  for (uint16_t i = 0; i < header.count; i++) {
    size_t bmPos =
        BANK_HEADER_SIZE +
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
      }
      sprites.push_back(std::move(img));
    } catch (...) {
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

  return bmpWriter::pixelsToBmp(sheetW, sheetH, sheet.data(), palette.data(),
                                static_cast<int>(palette.size()));
}

std::vector<std::vector<uint8_t>>
convertToIndividual(const std::vector<uint8_t> &data,
                    const std::vector<uint16_t> &palette) {
  auto header = parseHeader(data);

  std::vector<std::vector<uint8_t>> results;
  results.reserve(header.count);

  for (uint16_t i = 0; i < header.count; i++) {
    size_t bmPos =
        BANK_HEADER_SIZE +
        static_cast<size_t>(header.descriptors[i].wordOffset) * 2;

    try {
      auto img = decodeAmosBitmap(data, bmPos, palette.data(),
                                    static_cast<int>(palette.size()));
      if (!img.pixels.empty()) {
        results.push_back(bmpWriter::pixelsToBmp(
            img.width, img.height, img.pixels.data(), palette.data(),
            static_cast<int>(palette.size())));
      } else {
        results.push_back({});
      }
    } catch (...) {
      results.push_back({});
    }
  }

  return results;
}

std::vector<uint16_t> selectPalette(const std::string &fileId) {
  return palettes::selectPalette(fileId);
}

} // namespace openfranko::lib::converter::spriteSheet
