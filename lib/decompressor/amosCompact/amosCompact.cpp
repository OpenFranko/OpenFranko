#include "amosCompact.h"
#include "../helpers/helpers.h"
#include "Consts.h"
#include <stdexcept>

namespace openfranko::lib::decompressor::amosCompact {

namespace {
bool isSPACK(const std::vector<uint8_t> &data) {
  if (data.size() < consts::SPACK_HEADER_SIZE) {
    return false;
  }

  uint32_t header = helpers::readUint32BigEndian(data, 0);
  return header == consts::SPACK_SCREEN_HEADER;
}

bool isBitmap(const std::vector<uint8_t> &data) {
  if (data.size() < consts::BITMAP_HEADER_SIZE) {
    return false;
  }

  uint32_t header = helpers::readUint32BigEndian(data, 0);
  return header == consts::AMOS_BMCODE;
}

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
  std::vector<uint16_t> amigaPalette;
};

SPACKHeader parseSPACKHeader(const std::vector<uint8_t> &data) {
  SPACKHeader header;

  header.screenWidth = helpers::readUint16BigEndian(data, 4);
  header.screenHeight = helpers::readUint16BigEndian(data, 6);
  header.windowX = helpers::readUint16BigEndian(data, 8);
  header.windowY = helpers::readUint16BigEndian(data, 10);
  header.windowWidth = helpers::readUint16BigEndian(data, 12);
  header.windowHeight = helpers::readUint16BigEndian(data, 14);
  header.viewX = helpers::readUint16BigEndian(data, 16);
  header.viewY = helpers::readUint16BigEndian(data, 18);
  header.displayModeFlags = helpers::readUint16BigEndian(data, 20);
  header.numberOfColors = helpers::readUint16BigEndian(data, 22);
  header.numberOfBitplanes = helpers::readUint16BigEndian(data, 24);

  size_t paletteStart = consts::SPACK_HEADER_SIZE;
  for (size_t i = 0; i < consts::SPACK_PALETTE_SIZE; ++i) {
    uint16_t color = helpers::readUint16BigEndian(data, paletteStart + i * 2);
    header.amigaPalette.push_back(color);
  }

  return header;
}

struct BitmapHeader {
  int16_t xOffset;
  int16_t yOffset;
  uint16_t bytesWidth;
  uint16_t rowsHeight;
  uint16_t tileHeight;
  uint16_t numberOfBitplanes;
  uint32_t offsetToByteTable;
  uint32_t offsetToPointerTable;
};

BitmapHeader parseBitmapHeader(const std::vector<uint8_t> &data) {
  BitmapHeader header;

  header.xOffset = helpers::readInt16BigEndian(data, 4);
  header.yOffset = helpers::readInt16BigEndian(data, 6);
  header.bytesWidth = helpers::readUint16BigEndian(data, 8);
  header.rowsHeight = helpers::readUint16BigEndian(data, 10);
  header.tileHeight = helpers::readUint16BigEndian(data, 12);
  header.numberOfBitplanes = helpers::readUint16BigEndian(data, 14);
  header.offsetToByteTable = helpers::readUint32BigEndian(data, 16);
  header.offsetToPointerTable = helpers::readUint32BigEndian(data, 20);

  return header;
}

std::vector<uint8_t> decompressSPACK(const std::vector<uint8_t> &data) {
  SPACKHeader header = parseSPACKHeader(data);
  return {};
}

std::vector<uint8_t> decompressBitmap(const std::vector<uint8_t> &data) {
  BitmapHeader header = parseBitmapHeader(data);
  return {};
}

} // namespace

std::vector<uint8_t> decompress(const std::vector<uint8_t> &compressedData) {
  if (compressedData.size() < consts::MINIMAL_SIZE) {
    throw std::runtime_error("File is too small");
  }

  if (isSPACK(compressedData)) {
    return decompressSPACK(compressedData);
  }

  if (isBitmap(compressedData)) {
    return decompressBitmap(compressedData);
  }

  return {};
}

} // namespace openfranko::lib::decompressor::amosCompact