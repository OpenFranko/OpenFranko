#include "bmpWriter.h"

#include "../binary/binary.h"

namespace openfranko::lib::bmpWriter {
namespace {

constexpr uint32_t BMP_FILE_HEADER_SIZE = 14;
constexpr uint32_t BMP_INFO_HEADER_SIZE = 40;
constexpr uint32_t BMP_PALETTE_ENTRY_SIZE = 4;
constexpr uint32_t BMP_PALETTE_ENTRY_COUNT = 256;
constexpr uint16_t BMP_PLANE_COUNT = 1;
constexpr uint16_t BMP_BITS_PER_PIXEL = 8;
constexpr uint32_t BMP_PIXELS_PER_METER_72DPI = 2835;

} // namespace

std::vector<uint8_t> pixelsToBmp(uint32_t width, uint32_t height,
                                 const uint8_t *pixels, const uint16_t *palette,
                                 int numberOfColors) {
  uint32_t bmpRowBytes = (width + 3) & ~3u;
  uint32_t pixelOffset = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE +
                         BMP_PALETTE_ENTRY_COUNT * BMP_PALETTE_ENTRY_SIZE;
  uint32_t pixelDataSize = bmpRowBytes * height;
  uint32_t fileSize = pixelOffset + pixelDataSize;

  if (numberOfColors > static_cast<int>(BMP_PALETTE_ENTRY_COUNT)) {
    numberOfColors = static_cast<int>(BMP_PALETTE_ENTRY_COUNT);
  }

  std::vector<uint8_t> buf;
  buf.reserve(fileSize);

  buf.push_back('B');
  buf.push_back('M');
  binary::pushLittleEndian32(buf, fileSize);
  binary::pushLittleEndian32(buf, 0);
  binary::pushLittleEndian32(buf, pixelOffset);

  binary::pushLittleEndian32(buf, BMP_INFO_HEADER_SIZE);
  binary::pushLittleEndian32(buf, width);
  binary::pushLittleEndian32(buf, height);
  binary::pushLittleEndian16(buf, BMP_PLANE_COUNT);
  binary::pushLittleEndian16(buf, BMP_BITS_PER_PIXEL);
  binary::pushLittleEndian32(buf, 0);
  binary::pushLittleEndian32(buf, pixelDataSize);
  binary::pushLittleEndian32(buf, BMP_PIXELS_PER_METER_72DPI);
  binary::pushLittleEndian32(buf, BMP_PIXELS_PER_METER_72DPI);
  binary::pushLittleEndian32(buf, static_cast<uint32_t>(numberOfColors));
  binary::pushLittleEndian32(buf, 0);

  for (uint32_t i = 0; i < BMP_PALETTE_ENTRY_COUNT; ++i) {
    if (static_cast<int>(i) < numberOfColors) {
      auto r = static_cast<uint8_t>(((palette[i] >> 8) & 0xF) * 17);
      auto g = static_cast<uint8_t>(((palette[i] >> 4) & 0xF) * 17);
      auto b = static_cast<uint8_t>((palette[i] & 0xF) * 17);
      buf.push_back(b);
      buf.push_back(g);
      buf.push_back(r);
      buf.push_back(0);
    } else {
      buf.insert(buf.end(), BMP_PALETTE_ENTRY_SIZE, 0);
    }
  }

  for (int y = static_cast<int>(height) - 1; y >= 0; --y) {
    const uint8_t *row = pixels + y * width;
    buf.insert(buf.end(), row, row + width);
    if (bmpRowBytes > width) {
      buf.insert(buf.end(), bmpRowBytes - width, 0);
    }
  }

  return buf;
}

} // namespace openfranko::lib::bmpWriter
