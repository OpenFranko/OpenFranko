#include "IndexedSurface.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace openfranko::src::engine::street::core {
namespace {

constexpr int WORD_BYTES = 4;
constexpr int BYTE_BITS = 8;
constexpr uint32_t LOW_BITS = 0x01010101u;
constexpr uint32_t HIGH_BITS = 0x80808080u;

bool hasZeroByte(uint32_t word) {
  return ((word - LOW_BITS) & ~word & HIGH_BITS) != 0;
}

uint32_t swapBytes(uint32_t word) {
  return word >> 24 | (word >> 8 & 0xFF00u) | (word << 8 & 0xFF0000u) |
         word << 24;
}

int clampToSize(int value, int size) {
  if (value < 0) {
    return 0;
  }
  return std::min(value, size);
}

} // namespace

IndexedSurface::IndexedSurface(int width, int height)
    : m_width(width), m_height(height),
      m_pixels(static_cast<std::size_t>(width * height), 0) {}

int IndexedSurface::width() const { return m_width; }

int IndexedSurface::height() const { return m_height; }

uint8_t IndexedSurface::pixel(int x, int y) const {
  return m_pixels[static_cast<std::size_t>(y * m_width + x)];
}

const std::vector<uint8_t> &IndexedSurface::pixels() const { return m_pixels; }

void IndexedSurface::fill(uint8_t color) {
  std::fill(m_pixels.begin(), m_pixels.end(), color);
}

void IndexedSurface::clear(uint8_t color, int x1, int y1, int x2, int y2) {
  x1 = clampToSize(x1, m_width);
  y1 = clampToSize(y1, m_height);
  x2 = clampToSize(x2, m_width);
  y2 = clampToSize(y2, m_height);
  if (x2 <= x1 || y2 <= y1) {
    return;
  }
  for (int y = y1; y < y2; ++y) {
    auto row = m_pixels.begin() + y * m_width;
    std::fill(row + x1, row + x2, color);
  }
}

void IndexedSurface::copy(const IndexedSurface &source, int x1, int y1, int x2,
                          int y2, int x, int y) {
  if (x1 < 0) {
    x -= x1;
    x1 = 0;
  }
  if (y1 < 0) {
    y -= y1;
    y1 = 0;
  }
  if (x < 0) {
    x1 -= x;
    x = 0;
  }
  if (y < 0) {
    y1 -= y;
    y = 0;
  }
  if (x1 >= source.m_width || y1 >= source.m_height || x >= m_width ||
      y >= m_height || x2 < 0 || y2 < 0) {
    return;
  }
  int width = std::min(x2, source.m_width) - x1;
  int height = std::min(y2, source.m_height) - y1;
  if (width <= 0 || height <= 0) {
    return;
  }
  width = std::min(width, m_width - x);
  height = std::min(height, m_height - y);

  const bool upward = &source == this && y > y1;
  for (int step = 0; step < height; ++step) {
    const int row = upward ? height - 1 - step : step;
    std::memmove(
        m_pixels.data() + static_cast<std::ptrdiff_t>(y + row) * m_width + x,
        source.m_pixels.data() +
            static_cast<std::ptrdiff_t>(y1 + row) * source.m_width + x1,
        static_cast<std::size_t>(width));
  }
}

void IndexedSurface::unpack(const Picture &picture, int x, int y) {
  if (x < 0 || y < 0) {
    throw std::out_of_range("Unpack: the picture does not fit the screen");
  }
  x -= x % 8;
  if (x + picture.width > m_width || y + picture.height > m_height) {
    throw std::out_of_range("Unpack: the picture does not fit the screen");
  }
  for (int row = 0; row < picture.height; ++row) {
    const auto from = picture.pixels.begin() + row * picture.width;
    std::copy(from, from + picture.width,
              m_pixels.begin() + (y + row) * m_width + x);
  }
}

bool IndexedSurface::intersects(int left, int top, int width,
                                int height) const {
  return left < m_width && top < m_height && left + width > 0 &&
         top + height > 0;
}

void IndexedSurface::draw(const Picture &picture, int left, int top, bool flipX,
                          bool flipY, bool opaque) {
  const int firstColumn = std::max(0, -left);
  const int lastColumn = std::min(picture.width, m_width - left);
  const int firstRow = std::max(0, -top);
  const int lastRow = std::min(picture.height, m_height - top);
  if (firstColumn >= lastColumn || firstRow >= lastRow) {
    return;
  }
  for (int row = firstRow; row < lastRow; ++row) {
    const int sourceRow = flipY ? picture.height - 1 - row : row;
    const uint8_t *source =
        picture.pixels.data() +
        static_cast<std::ptrdiff_t>(sourceRow) * picture.width;
    uint8_t *target =
        m_pixels.data() + static_cast<std::ptrdiff_t>(top + row) * m_width;
    if (opaque && !flipX) {
      std::copy(source + firstColumn, source + lastColumn,
                target + left + firstColumn);
      continue;
    }
    int column = firstColumn;
    for (; column + WORD_BYTES <= lastColumn; column += WORD_BYTES) {
      uint32_t word = 0;
      if (flipX) {
        std::memcpy(&word, source + picture.width - WORD_BYTES - column,
                    WORD_BYTES);
        word = swapBytes(word);
      } else {
        std::memcpy(&word, source + column, WORD_BYTES);
      }
      if (opaque || !hasZeroByte(word)) {
        std::memcpy(target + left + column, &word, WORD_BYTES);
        continue;
      }
      for (int byte = 0; byte < WORD_BYTES && word != 0; ++byte) {
        const uint8_t value = static_cast<uint8_t>(word);
        if (value != 0) {
          target[left + column + byte] = value;
        }
        word >>= BYTE_BITS;
      }
    }
    for (; column < lastColumn; ++column) {
      const uint8_t value = source[flipX ? picture.width - 1 - column : column];
      if (value != 0 || opaque) {
        target[left + column] = value;
      }
    }
  }
}

ScreenBlock::ScreenBlock(const IndexedSurface &source, int x, int y, int width,
                         int height)
    : m_pixels(width, height), m_x(x), m_y(y) {
  m_pixels.copy(source, x, y, x + width, y + height, 0, 0);
}

void ScreenBlock::put(IndexedSurface &target) const {
  target.copy(m_pixels, 0, 0, m_pixels.width(), m_pixels.height(), m_x, m_y);
}

} // namespace openfranko::src::engine::street::core
