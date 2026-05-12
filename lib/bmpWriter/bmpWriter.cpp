#include "bmpWriter.h"

namespace openfranko::lib::bmpWriter {

namespace {

void pushLittleEndian16(std::vector<uint8_t> &buf, uint16_t v) {
  buf.push_back(static_cast<uint8_t>(v));
  buf.push_back(static_cast<uint8_t>(v >> 8));
}

void pushLittleEndian32(std::vector<uint8_t> &buf, uint32_t v) {
  buf.push_back(static_cast<uint8_t>(v));
  buf.push_back(static_cast<uint8_t>(v >> 8));
  buf.push_back(static_cast<uint8_t>(v >> 16));
  buf.push_back(static_cast<uint8_t>(v >> 24));
}

} // namespace

std::vector<uint8_t> pixelsToBmp(uint32_t width, uint32_t height,
                                 const uint8_t *pixels, const uint16_t *palette,
                                 int numberOfColors) {
  uint32_t bmpRowBytes = (width + 3) & ~3u;
  uint32_t paletteEntries = 256;
  uint32_t pixelOffset = 14 + 40 + paletteEntries * 4;
  uint32_t pixelDataSize = bmpRowBytes * height;
  uint32_t fileSize = pixelOffset + pixelDataSize;

  if (numberOfColors > 256) {
    numberOfColors = 256;
  }

  std::vector<uint8_t> buf;
  buf.reserve(fileSize);

  buf.push_back('B');
  buf.push_back('M');
  pushLittleEndian32(buf, fileSize);
  pushLittleEndian32(buf, 0);
  pushLittleEndian32(buf, pixelOffset);

  pushLittleEndian32(buf, 40);
  pushLittleEndian32(buf, width);
  pushLittleEndian32(buf, height);
  pushLittleEndian16(buf, 1);
  pushLittleEndian16(buf, 8);
  pushLittleEndian32(buf, 0);
  pushLittleEndian32(buf, pixelDataSize);
  pushLittleEndian32(buf, 2835);
  pushLittleEndian32(buf, 2835);
  pushLittleEndian32(buf, static_cast<uint32_t>(numberOfColors));
  pushLittleEndian32(buf, 0);

  for (uint32_t i = 0; i < paletteEntries; i++) {
    if (static_cast<int>(i) < numberOfColors) {
      auto r = static_cast<uint8_t>(((palette[i] >> 8) & 0xF) * 17);
      auto g = static_cast<uint8_t>(((palette[i] >> 4) & 0xF) * 17);
      auto b = static_cast<uint8_t>((palette[i] & 0xF) * 17);
      buf.push_back(b);
      buf.push_back(g);
      buf.push_back(r);
      buf.push_back(0);
    } else {
      buf.insert(buf.end(), 4, 0);
    }
  }

  for (int y = static_cast<int>(height) - 1; y >= 0; y--) {
    const uint8_t *row = pixels + y * width;
    buf.insert(buf.end(), row, row + width);
    if (bmpRowBytes > width) {
      buf.insert(buf.end(), bmpRowBytes - width, 0);
    }
  }

  return buf;
}

} // namespace openfranko::lib::bmpWriter
