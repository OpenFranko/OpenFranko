#include "decodeImage.h"
#include "../../helpers/helpers.h"
#include "../amosCompact/Consts.h"
#include "../amosCompact/detail/bitmapUnpack.h"
#include "headers.h"
#include <stdexcept>

namespace openfranko::lib::converter {

DecodedImage decodeAmosBitmap(const std::vector<uint8_t> &data, size_t offset,
                              const uint16_t *palette, int numberOfColors) {
  if (offset > data.size() ||
      data.size() - offset < amosCompact::consts::PACKED_BITMAP_HEADER_SIZE) {
    throw std::runtime_error("Data too small for bitmap header");
  }
  if (palette == nullptr || numberOfColors <= 0) {
    throw std::runtime_error("A palette is required to decode a bitmap");
  }
  if (helpers::BigEndianReader(data).readUint32(offset) !=
      amosCompact::consts::AMOS_BMCODE) {
    throw std::runtime_error("Invalid bitmap magic number");
  }

  std::vector<uint8_t> slice(data.begin() + static_cast<std::ptrdiff_t>(offset),
                             data.end());
  std::vector<uint16_t> palVec(palette, palette + numberOfColors);

  auto hdr = headers::parseBitmapHeader(slice);
  auto bm = amosCompact::detail::bitmapUnpack(slice, hdr, palVec);

  DecodedImage img;
  img.width = bm.width;
  img.height = bm.height;
  if (!bm.chunkyPixels.empty() && bm.width > 0 && bm.height > 0) {
    size_t n = static_cast<size_t>(bm.width) * bm.height;
    if (bm.chunkyPixels.size() < n) {
      throw std::runtime_error(
          "Unpacked bitmap is smaller than its dimensions");
    }
    img.pixels.assign(bm.chunkyPixels.begin(), bm.chunkyPixels.begin() + n);
  }

  return img;
}

} // namespace openfranko::lib::converter
