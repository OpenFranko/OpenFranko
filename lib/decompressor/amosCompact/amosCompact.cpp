#include "amosCompact.h"
#include "../helpers/helpers.h"
#include "Consts.h"
#include "SPACKScreen.h"
#include "packedBitmap.h"
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

std::vector<uint8_t> decompressSPACK(const std::vector<uint8_t> &data) {
  SPACKScreen screen(data);
  return screen.getData();
}

std::vector<uint8_t> decompressPackedBitmap(const std::vector<uint8_t> &data) {
  PackedBitmap bitmap(data);
  return bitmap.getData();
}

} // namespace

std::vector<uint8_t> decompress(const std::vector<uint8_t> &compressedData) {
  if (compressedData.size() < consts::MINIMAL_SIZE) {
    throw std::runtime_error("File is too small");
  }

  if (isSPACK(compressedData)) {
    return decompressSPACK(compressedData);
  }

  if (isPackedBitmap(compressedData)) {
    return decompressPackedBitmap(compressedData);
  }

  return {};
}

} // namespace openfranko::lib::decompressor::amosCompact