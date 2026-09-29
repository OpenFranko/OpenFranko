#include "amosCompact.h"

#include "../../binary/binary.h"
#include "../../bmpWriter/bmpWriter.h"
#include "../headers/headers.h"
#include "consts.h"
#include "detail/unpackBitmap.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace openfranko::lib::converter::amosCompact {
namespace {

bool isSpack(const std::vector<uint8_t> &data) {
  uint32_t header = binary::BigEndianReader(data).readUint32(0);
  return header == headers::SPACK_SCREEN_HEADER;
}

bool isPackedBitmap(const std::vector<uint8_t> &data) {
  uint32_t header = binary::BigEndianReader(data).readUint32(0);
  return header == headers::AMOS_BMCODE;
}

std::vector<uint16_t> defaultPalette(uint16_t numberOfBitplanes) {
  const int numberOfColors = std::min(
      1 << numberOfBitplanes, static_cast<int>(headers::SPACK_COLOR_COUNT));

  std::vector<uint16_t> palette(headers::SPACK_COLOR_COUNT, 0);
  for (int i = 0; i < numberOfColors; ++i) {
    const auto level = static_cast<uint16_t>(i * 15 / (numberOfColors - 1));
    palette[i] = static_cast<uint16_t>(level * 0x111);
  }
  return palette;
}

} // namespace

std::vector<uint8_t> decompress(const std::vector<uint8_t> &compressedData) {
  if (compressedData.size() < consts::MINIMAL_SIZE) {
    throw std::runtime_error("File is too small");
  }

  std::vector<uint8_t> data = compressedData;
  std::vector<uint16_t> palette;

  if (isSpack(data)) {
    if (data.size() <
        headers::SPACK_HEADER_SIZE + headers::PACKED_BITMAP_HEADER_SIZE) {
      throw std::runtime_error("File is too small to be a valid SPACK screen");
    }

    auto spackHeader = headers::parseSpackHeader(data);
    palette = std::vector<uint16_t>(std::begin(spackHeader.amigaPalette),
                                    std::end(spackHeader.amigaPalette));
    data = std::vector<uint8_t>(data.begin() + headers::SPACK_HEADER_SIZE,
                                data.end());
  }

  if (!isPackedBitmap(data) ||
      data.size() < headers::PACKED_BITMAP_HEADER_SIZE) {
    throw std::runtime_error("File is not a valid packed bitmap");
  }

  auto bitmapHeader = headers::parseBitmapHeader(data);
  auto unpackedBitmap = detail::unpackBitmap(data, bitmapHeader);

  if (palette.empty()) {
    palette = defaultPalette(unpackedBitmap.numberOfBitplanes);
  }

  const int numberOfColors = 1 << unpackedBitmap.numberOfBitplanes;
  if (numberOfColors > static_cast<int>(headers::SPACK_COLOR_COUNT)) {
    palette.resize(static_cast<std::size_t>(numberOfColors));
    for (std::size_t i = headers::SPACK_COLOR_COUNT; i < palette.size(); ++i) {
      const uint16_t base = palette[i - headers::SPACK_COLOR_COUNT];
      palette[i] = static_cast<uint16_t>((base >> 1) & 0x777);
    }
  }

  return bmpWriter::pixelsToBmp(unpackedBitmap.width, unpackedBitmap.height,
                                unpackedBitmap.chunkyPixels.data(),
                                palette.data(), numberOfColors);
}

} // namespace openfranko::lib::converter::amosCompact
