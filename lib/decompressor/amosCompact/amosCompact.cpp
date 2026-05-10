#include "amosCompact.h"
#include "../helpers/helpers.h"
#include "Consts.h"
#include "SPACKScreen.h"
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
  SPACKScreen screen(data);
  return screen.getData();
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