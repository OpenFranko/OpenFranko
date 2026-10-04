#include "graphics/Bitmap.h"

#include "graphics/Display.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace openfranko::src::systems::graphics {
namespace {

constexpr std::size_t HOTSPOT_X = 6;
constexpr std::size_t HOTSPOT_Y_OFFSET = 8;
constexpr std::size_t PIXEL_OFFSET = 10;
constexpr std::size_t FILE_HEADER_SIZE = 14;
constexpr std::size_t WIDTH_OFFSET = 18;
constexpr std::size_t HEIGHT_OFFSET = 22;
constexpr std::size_t BITS_OFFSET = 28;
constexpr std::size_t COMPRESSION_OFFSET = 30;
constexpr std::size_t COLORS_USED_OFFSET = 46;

constexpr uint32_t INFO_HEADER_SIZE = 40;
constexpr uint32_t INDEXED_BITS = 8;
constexpr uint32_t UNCOMPRESSED = 0;
constexpr uint32_t MAX_COLORS = 256;
constexpr std::size_t PALETTE_ENTRY_SIZE = 4;
constexpr std::size_t ROW_ALIGNMENT = 4;

[[noreturn]] void fail(const std::string &cause) {
  throw std::runtime_error(cause);
}

struct FileBytes {
  const uint8_t *data = nullptr;
  std::size_t size = 0;
};

const uint8_t *bytes(const FileBytes &file, std::size_t offset,
                     std::size_t size) {
  if (offset > file.size || size > file.size - offset) {
    fail("Truncated bitmap");
  }
  return file.data + offset;
}

uint32_t readLittleEndian(const FileBytes &file, std::size_t offset,
                          std::size_t size) {
  const uint8_t *field = bytes(file, offset, size);
  uint32_t value = 0;
  for (std::size_t i = size; i > 0; --i) {
    value = value << 8 | field[i - 1];
  }
  return value;
}

constexpr std::array<uint8_t, 256> NIBBLES = [] {
  std::array<uint8_t, 256> nibbles{};
  for (std::size_t channel = 0; channel < nibbles.size(); ++channel) {
    nibbles[channel] =
        static_cast<uint8_t>((channel + CHANNEL_STEP / 2) / CHANNEL_STEP);
  }
  return nibbles;
}();

int toNibble(uint8_t channel) { return NIBBLES[channel]; }

uint16_t toAmigaColor(const uint8_t *blueGreenRed) {
  return static_cast<uint16_t>(toNibble(blueGreenRed[2]) << 8 |
                               toNibble(blueGreenRed[1]) << 4 |
                               toNibble(blueGreenRed[0]));
}

} // namespace

IndexedBitmap readIndexedBitmap(const uint8_t *data, std::size_t size) {
  const FileBytes file{data, size};
  if (file.size < 2 || file.data[0] != 'B' || file.data[1] != 'M') {
    fail("Not a bitmap");
  }
  const uint32_t headerSize = readLittleEndian(file, FILE_HEADER_SIZE, 4);
  if (headerSize < INFO_HEADER_SIZE) {
    fail("Unsupported bitmap header");
  }
  if (readLittleEndian(file, BITS_OFFSET, 2) != INDEXED_BITS ||
      readLittleEndian(file, COMPRESSION_OFFSET, 4) != UNCOMPRESSED) {
    fail("Not an 8-bit indexed bitmap");
  }
  const auto width = static_cast<int64_t>(
      static_cast<int32_t>(readLittleEndian(file, WIDTH_OFFSET, 4)));
  const auto height = static_cast<int64_t>(
      static_cast<int32_t>(readLittleEndian(file, HEIGHT_OFFSET, 4)));
  const int64_t rows = height < 0 ? -height : height;
  if (width <= 0 || rows == 0) {
    fail("Empty bitmap");
  }
  const uint32_t colorsUsed = readLittleEndian(file, COLORS_USED_OFFSET, 4);
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
  bitmap.hotspotY =
      static_cast<int>(readLittleEndian(file, HOTSPOT_Y_OFFSET, 2));

  const uint8_t *entry =
      bytes(file, FILE_HEADER_SIZE + headerSize, colors * PALETTE_ENTRY_SIZE);
  bitmap.palette.resize(colors);
  for (uint16_t &color : bitmap.palette) {
    color = toAmigaColor(entry);
    entry += PALETTE_ENTRY_SIZE;
  }

  bitmap.pixels.resize(static_cast<std::size_t>(width * rows));
  const std::size_t rowBytes = static_cast<std::size_t>(width);
  std::size_t source =
      pixelStart +
      (height < 0 ? 0 : stride * static_cast<std::size_t>(rows - 1));
  uint8_t *target = bitmap.pixels.data();
  for (int row = 0; row < bitmap.height; ++row) {
    std::memcpy(target, file.data + source, rowBytes);
    target += rowBytes;
    source = height < 0 ? source + stride : source - stride;
  }
  return bitmap;
}

IndexedBitmap readIndexedBitmap(const std::vector<uint8_t> &file) {
  return readIndexedBitmap(file.data(), file.size());
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

IndexedBitmap mirrored(const IndexedBitmap &image) {
  IndexedBitmap flipped = image;
  flipped.hotspotX = image.width - image.hotspotX;
  for (std::size_t row = 0; row < static_cast<std::size_t>(image.height);
       ++row) {
    const auto start = flipped.pixels.begin() +
                       static_cast<std::ptrdiff_t>(
                           row * static_cast<std::size_t>(image.width));
    std::reverse(start, start + image.width);
  }
  return flipped;
}

} // namespace openfranko::src::systems::graphics
