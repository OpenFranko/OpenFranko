#include "decodeAmosBitmap.h"
#include "../../binary/binary.h"
#include "../headers/headers.h"
#include "detail/unpackBitmap.h"

#include <stdexcept>

namespace openfranko::lib::converter::amosCompact {

DecodedImage decodeAmosBitmap(const std::vector<uint8_t> &data, size_t offset,
                              const uint16_t *palette, int numberOfColors) {
  if (offset > data.size() ||
      data.size() - offset < headers::PACKED_BITMAP_HEADER_SIZE) {
    throw std::runtime_error("Data too small for bitmap header");
  }
  if (palette == nullptr || numberOfColors <= 0) {
    throw std::runtime_error("A palette is required to decode a bitmap");
  }
  if (binary::BigEndianReader(data).readUint32(offset) !=
      headers::AMOS_BMCODE) {
    throw std::runtime_error("Invalid bitmap magic number");
  }

  std::vector<uint8_t> slice(data.begin() + static_cast<std::ptrdiff_t>(offset),
                             data.end());
  std::vector<uint16_t> paletteColors(palette, palette + numberOfColors);

  auto header = headers::parseBitmapHeader(slice);
  auto bitmap = detail::unpackBitmap(slice, header, paletteColors);

  DecodedImage image;
  image.width = bitmap.width;
  image.height = bitmap.height;
  if (!bitmap.chunkyPixels.empty() && bitmap.width > 0 && bitmap.height > 0) {
    size_t n = static_cast<size_t>(bitmap.width) * bitmap.height;
    if (bitmap.chunkyPixels.size() < n) {
      throw std::runtime_error(
          "Unpacked bitmap is smaller than its dimensions");
    }
    image.pixels.assign(bitmap.chunkyPixels.begin(),
                        bitmap.chunkyPixels.begin() + n);
  }

  return image;
}

} // namespace openfranko::lib::converter::amosCompact
