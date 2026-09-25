#include "IndexedSurface.h"

#include <algorithm>
#include <stdexcept>

namespace openfranko::src::engine::street {
namespace {

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

  std::vector<uint8_t> block(static_cast<std::size_t>(width * height));
  for (int row = 0; row < height; ++row) {
    const auto from =
        source.m_pixels.begin() + (y1 + row) * source.m_width + x1;
    std::copy(from, from + width, block.begin() + row * width);
  }
  for (int row = 0; row < height; ++row) {
    const auto from = block.begin() + row * width;
    std::copy(from, from + width, m_pixels.begin() + (y + row) * m_width + x);
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
  for (int row = 0; row < picture.height; ++row) {
    const int y = top + row;
    if (y < 0 || y >= m_height) {
      continue;
    }
    const int sourceRow = flipY ? picture.height - 1 - row : row;
    for (int column = 0; column < picture.width; ++column) {
      const int x = left + column;
      if (x < 0 || x >= m_width) {
        continue;
      }
      const int sourceColumn = flipX ? picture.width - 1 - column : column;
      const uint8_t value = picture.at(sourceColumn, sourceRow);
      if (value != 0 || opaque) {
        m_pixels[static_cast<std::size_t>(y * m_width + x)] = value;
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

} // namespace openfranko::src::engine::street
