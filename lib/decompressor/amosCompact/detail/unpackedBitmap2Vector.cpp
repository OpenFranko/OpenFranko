#include "unpackedBitmap2Vector.h"
#include <stdexcept>

namespace openfranko::lib::decompressor::amosCompact::detail {

namespace {

void pushLittleEndianUInt16(std::vector<uint8_t> &buf, uint16_t v) {
  buf.push_back(static_cast<uint8_t>(v));
  buf.push_back(static_cast<uint8_t>(v >> 8));
}

void pushLittleEndianUInt32(std::vector<uint8_t> &buf, uint32_t v) {
  buf.push_back(static_cast<uint8_t>(v));
  buf.push_back(static_cast<uint8_t>(v >> 8));
  buf.push_back(static_cast<uint8_t>(v >> 16));
  buf.push_back(static_cast<uint8_t>(v >> 24));
}

void rgb4ToRgb8(uint16_t color12, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = static_cast<uint8_t>(((color12 >> 8) & 0xF) * 17);
  g = static_cast<uint8_t>(((color12 >> 4) & 0xF) * 17);
  b = static_cast<uint8_t>((color12 & 0xF) * 17);
}

} // namespace

std::vector<uint8_t> unpackedBitmap2Vector(const UnpackedBitmap &bitmap) {
  if (bitmap.width == 0 || bitmap.height == 0) {
    throw std::runtime_error("Bitmap has zero dimensions");
  }
  if (!bitmap.chunkyPixels) {
    throw std::runtime_error("Bitmap has no chunky pixel data");
  }

  uint32_t bmpRowBytes = (bitmap.width + 3) & ~3u;
  uint32_t paletteEntries = 256;
  uint32_t pixelOffset = 14 + 40 + paletteEntries * 4;
  uint32_t pixelDataSize = bmpRowBytes * bitmap.height;
  uint32_t fileSize = pixelOffset + pixelDataSize;

  int ncolors = 1 << bitmap.numberOfBitplanes;
  if (ncolors > 32)
    ncolors = 32;

  std::vector<uint8_t> buf;
  buf.reserve(fileSize);

  buf.push_back('B');
  buf.push_back('M');
  pushLittleEndianUInt32(buf, fileSize);
  pushLittleEndianUInt32(buf, 0);
  pushLittleEndianUInt32(buf, pixelOffset);

  pushLittleEndianUInt32(buf, 40);
  pushLittleEndianUInt32(buf, bitmap.width);
  pushLittleEndianUInt32(buf, bitmap.height);
  pushLittleEndianUInt16(buf, 1);
  pushLittleEndianUInt16(buf, 8);
  pushLittleEndianUInt32(buf, 0);
  pushLittleEndianUInt32(buf, pixelDataSize);
  pushLittleEndianUInt32(buf, 2835);
  pushLittleEndianUInt32(buf, 2835);
  pushLittleEndianUInt32(buf, static_cast<uint32_t>(ncolors));
  pushLittleEndianUInt32(buf, 0);

  for (uint32_t i = 0; i < paletteEntries; i++) {
    if (static_cast<int>(i) < ncolors) {
      uint8_t r, g, b;
      rgb4ToRgb8(bitmap.palette[i], r, g, b);
      buf.push_back(b);
      buf.push_back(g);
      buf.push_back(r);
      buf.push_back(0);
    } else {
      buf.insert(buf.end(), 4, 0);
    }
  }

  for (int y = bitmap.height - 1; y >= 0; y--) {
    const uint8_t *row = bitmap.chunkyPixels + y * bitmap.width;
    buf.insert(buf.end(), row, row + bitmap.width);
    if (bmpRowBytes > bitmap.width) {
      buf.insert(buf.end(), bmpRowBytes - bitmap.width, 0);
    }
  }

  return buf;
}

} // namespace openfranko::lib::decompressor::amosCompact::detail
