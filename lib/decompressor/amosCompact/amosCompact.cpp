#include "amosCompact.h"
#include "../helpers/helpers.h"
#include "Consts.h"
#include "bitmapUnpack.h"
#include <stdexcept>

namespace openfranko::lib::decompressor::amosCompact {

namespace {
bool isSPACK(const std::vector<uint8_t> &data) {
  uint32_t header = helpers::readUint32BigEndian(data, 0);
  return header == consts::SPACK_SCREEN_HEADER;
}

bool isPackedBitmap(const std::vector<uint8_t> &data) {
  uint32_t header = helpers::readUint32BigEndian(data, 0);
  return header == consts::AMOS_BMCODE;
}

} // namespace

UnpackedBitmap decompress(const std::vector<uint8_t> &compressedData) {
  if (compressedData.size() < consts::MINIMAL_SIZE) {
    throw std::runtime_error("File is too small");
  }

  std::vector<uint8_t> data = compressedData;

  std::vector<uint16_t> palette = {0x555, 0xAAA, 0x666, 0xFAA, 0x083, 0x902,
                                   0xB95, 0x760, 0x063, 0x000, 0x520, 0x17A,
                                   0x09E, 0x4DF, 0x777, 0xDDD, 0xFFF};

  if (isSPACK(data)) {
    if (data.size() <
        consts::SPACK_HEADER_SIZE + consts::PACKED_BITMAP_HEADER_SIZE) {
      throw std::runtime_error("File is too small to be a valid SPACK screen");
    }

    auto spackHeader = headers::parseSPACKHeader(data);
    palette = std::vector<uint16_t>(std::begin(spackHeader.amigaPalette),
                                    std::end(spackHeader.amigaPalette));
    data = std::vector<uint8_t>(data.begin() + consts::SPACK_HEADER_SIZE,
                                data.end());
  }

  if (!isPackedBitmap(data) ||
      data.size() < consts::PACKED_BITMAP_HEADER_SIZE) {
    throw std::runtime_error("File is not a valid packed bitmap");
  }

  auto bitmapHeader = headers::parseBitmapHeader(data);

  return bitmapUnpack(data, bitmapHeader, palette);
}

} // namespace openfranko::lib::decompressor::amosCompact