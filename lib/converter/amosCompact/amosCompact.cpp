#include "amosCompact.h"
#include "../../bmpWriter/bmpWriter.h"
#include "../../helpers/helpers.h"
#include "Consts.h"
#include "detail/bitmapUnpack.h"
#include <stdexcept>

namespace openfranko::lib::converter::amosCompact {

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

std::vector<uint8_t> decompress(const std::vector<uint8_t> &compressedData) {
  if (compressedData.size() < consts::MINIMAL_SIZE) {
    throw std::runtime_error("File is too small");
  }

  std::vector<uint8_t> data = compressedData;

  std::vector<uint16_t> palette(32);
  for (int i = 0; i < 32; i++) {
    palette[i] = static_cast<uint16_t>((i * 0x111) & 0xFFF);
  }

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

  if (bitmapHeader.numberOfBitplanes == 0 ||
      bitmapHeader.numberOfBitplanes > consts::MAX_SUPPORTED_BITPLANES) {
    throw std::runtime_error("Invalid number of bitplanes");
  }
  if (bitmapHeader.gridX == 0 || bitmapHeader.gridY == 0 ||
      bitmapHeader.tileHeight == 0) {
    throw std::runtime_error("Invalid bitmap dimensions");
  }

  auto unpackedBitmap = detail::bitmapUnpack(data, bitmapHeader, palette);

  if (unpackedBitmap.width == 0 || unpackedBitmap.height == 0) {
    throw std::runtime_error("Bitmap has zero dimensions");
  }
  if (unpackedBitmap.chunkyPixels.empty()) {
    throw std::runtime_error("Bitmap has no chunky pixel data");
  }

  int numberOfColors = 1 << unpackedBitmap.numberOfBitplanes;
  if (numberOfColors > 32) {
    numberOfColors = 32;
  }

  return bmpWriter::pixelsToBmp(unpackedBitmap.width, unpackedBitmap.height,
                                unpackedBitmap.chunkyPixels.data(),
                                unpackedBitmap.palette, numberOfColors);
}

} // namespace openfranko::lib::converter::amosCompact