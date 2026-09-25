#include "Bitmap.h"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace openfranko::src::systems {
namespace {

constexpr std::size_t HOTSPOT_X = 6;
constexpr std::size_t HOTSPOT_Y = 8;
constexpr std::size_t PIXEL_OFFSET = 10;
constexpr std::size_t FILE_HEADER_SIZE = 14;
constexpr std::size_t WIDTH = 18;
constexpr std::size_t HEIGHT = 22;
constexpr std::size_t BITS = 28;
constexpr std::size_t COMPRESSION = 30;
constexpr std::size_t COLORS_USED = 46;

constexpr uint32_t INFO_HEADER_SIZE = 40;
constexpr uint32_t INDEXED_BITS = 8;
constexpr uint32_t UNCOMPRESSED = 0;
constexpr uint32_t MAX_COLORS = 256;
constexpr std::size_t PALETTE_ENTRY = 4;
constexpr std::size_t ROW_ALIGNMENT = 4;
constexpr int CHANNEL_STEP = 17;

[[noreturn]] void fail(const std::string &cause) {
  throw std::runtime_error(cause);
}

const uint8_t *bytes(const std::vector<uint8_t> &file, std::size_t offset,
                     std::size_t size) {
  if (offset > file.size() || size > file.size() - offset) {
    fail("Truncated bitmap");
  }
  return file.data() + offset;
}

uint32_t readLittleEndian(const std::vector<uint8_t> &file, std::size_t offset,
                          std::size_t size) {
  const uint8_t *field = bytes(file, offset, size);
  uint32_t value = 0;
  for (std::size_t i = size; i > 0; --i) {
    value = value << 8 | field[i - 1];
  }
  return value;
}

int toNibble(uint8_t channel) {
  return (channel + CHANNEL_STEP / 2) / CHANNEL_STEP;
}

uint16_t toAmigaColor(const uint8_t *blueGreenRed) {
  return static_cast<uint16_t>(toNibble(blueGreenRed[2]) << 8 |
                               toNibble(blueGreenRed[1]) << 4 |
                               toNibble(blueGreenRed[0]));
}

} // namespace

IndexedBitmap readIndexedBitmap(const std::vector<uint8_t> &file) {
  if (file.size() < 2 || file[0] != 'B' || file[1] != 'M') {
    fail("Not a bitmap");
  }
  const uint32_t headerSize = readLittleEndian(file, FILE_HEADER_SIZE, 4);
  if (headerSize < INFO_HEADER_SIZE) {
    fail("Unsupported bitmap header");
  }
  if (readLittleEndian(file, BITS, 2) != INDEXED_BITS ||
      readLittleEndian(file, COMPRESSION, 4) != UNCOMPRESSED) {
    fail("Not an 8-bit indexed bitmap");
  }
  const auto width = static_cast<int64_t>(
      static_cast<int32_t>(readLittleEndian(file, WIDTH, 4)));
  const auto height = static_cast<int64_t>(
      static_cast<int32_t>(readLittleEndian(file, HEIGHT, 4)));
  const int64_t rows = height < 0 ? -height : height;
  if (width <= 0 || rows == 0) {
    fail("Empty bitmap");
  }
  const uint32_t colorsUsed = readLittleEndian(file, COLORS_USED, 4);
  const uint32_t colors = colorsUsed == 0 ? MAX_COLORS : colorsUsed;
  if (colors > MAX_COLORS) {
    fail("Too many bitmap colours");
  }

  const std::size_t stride =
      (static_cast<std::size_t>(width) + ROW_ALIGNMENT - 1) / ROW_ALIGNMENT *
      ROW_ALIGNMENT;
  const std::size_t pixelStart = readLittleEndian(file, PIXEL_OFFSET, 4);
  bytes(file, pixelStart,
        stride * static_cast<std::size_t>(rows - 1) +
            static_cast<std::size_t>(width));

  IndexedBitmap bitmap;
  bitmap.width = static_cast<int>(width);
  bitmap.height = static_cast<int>(rows);
  bitmap.hotspotX = static_cast<int>(readLittleEndian(file, HOTSPOT_X, 2));
  bitmap.hotspotY = static_cast<int>(readLittleEndian(file, HOTSPOT_Y, 2));

  const std::size_t paletteStart = FILE_HEADER_SIZE + headerSize;
  for (uint32_t color = 0; color < colors; ++color) {
    bitmap.palette.push_back(toAmigaColor(
        bytes(file, paletteStart + color * PALETTE_ENTRY, PALETTE_ENTRY)));
  }

  bitmap.pixels.resize(static_cast<std::size_t>(width * rows));
  for (int row = 0; row < bitmap.height; ++row) {
    const int fileRow = height < 0 ? row : bitmap.height - 1 - row;
    const uint8_t *source =
        file.data() + pixelStart + stride * static_cast<std::size_t>(fileRow);
    std::copy(source, source + bitmap.width,
              bitmap.pixels.begin() +
                  static_cast<std::ptrdiff_t>(row) * bitmap.width);
  }
  return bitmap;
}

IndexedBitmap loadIndexedBitmap(const std::string &path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("Failed to load bitmap: " + path);
  }
  const std::vector<uint8_t> file{std::istreambuf_iterator<char>(stream),
                                  std::istreambuf_iterator<char>()};
  try {
    return readIndexedBitmap(file);
  } catch (const std::runtime_error &error) {
    throw std::runtime_error(std::string(error.what()) + ": " + path);
  }
}

} // namespace openfranko::src::systems
