#include "IndexedSurface.h"

#include "../../../systems/Multiply.h"
#include "../../../systems/graphics/Display.h"
#include "../../../systems/graphics/PixelOps.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace openfranko::src::engine::street::core {
namespace {

namespace pixels = systems::graphics::pixels;

std::ptrdiff_t rowOffset(int row, int width) {
  return systems::multiplySigned16(static_cast<int16_t>(row),
                                   static_cast<int16_t>(width));
}

const std::vector<uint8_t> &settled(const std::vector<uint8_t> &pixels) {
  pixels::finish();
  return pixels;
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
      m_pixels(static_cast<std::size_t>(width * height), 0),
      m_revision(systems::graphics::newRevision()) {}

IndexedSurface::IndexedSurface(int width, int height,
                               std::vector<uint8_t> pixels)
    : m_width(width), m_height(height), m_pixels(std::move(pixels)),
      m_revision(systems::graphics::newRevision()) {
  m_pixels.resize(static_cast<std::size_t>(width * height), 0);
}

IndexedSurface::IndexedSurface(const IndexedSurface &other)
    : m_width(other.m_width), m_height(other.m_height),
      m_pixels(settled(other.m_pixels)),
      m_revision(systems::graphics::newRevision()) {}

IndexedSurface &IndexedSurface::operator=(const IndexedSurface &other) {
  pixels::finish();
  m_width = other.m_width;
  m_height = other.m_height;
  m_pixels = other.m_pixels;
  m_revision = systems::graphics::newRevision();
  return *this;
}

IndexedSurface &IndexedSurface::operator=(IndexedSurface &&other) noexcept {
  if (!m_pixels.empty()) {
    pixels::finish();
  }
  m_width = other.m_width;
  m_height = other.m_height;
  m_pixels = std::move(other.m_pixels);
  m_revision = systems::graphics::newRevision();
  return *this;
}

IndexedSurface::~IndexedSurface() {
  if (!m_pixels.empty()) {
    pixels::finish();
  }
}

int IndexedSurface::width() const { return m_width; }

int IndexedSurface::height() const { return m_height; }

uint8_t IndexedSurface::pixel(int x, int y) const {
  pixels::finish();
  return m_pixels[static_cast<std::size_t>(y * m_width + x)];
}

const std::vector<uint8_t> &IndexedSurface::pixels() const { return m_pixels; }

uint32_t IndexedSurface::revision() const { return m_revision; }

void IndexedSurface::reshape(int width, int height) {
  m_revision = systems::graphics::newRevision();
  const std::size_t size = static_cast<std::size_t>(rowOffset(height, width));
  if (size > m_pixels.size()) {
    if (size > m_pixels.capacity()) {
      pixels::finish();
    }
    m_pixels.resize(size);
  }
  m_width = width;
  m_height = height;
}

void IndexedSurface::fill(uint8_t color) {
  m_revision = systems::graphics::newRevision();
  pixels::fill(pixels::Target{m_pixels.data(), m_width}, m_width, m_height,
               color);
}

void IndexedSurface::clear(uint8_t color, int x1, int y1, int x2, int y2) {
  m_revision = systems::graphics::newRevision();
  x1 = clampToSize(x1, m_width);
  y1 = clampToSize(y1, m_height);
  x2 = clampToSize(x2, m_width);
  y2 = clampToSize(y2, m_height);
  if (x2 <= x1 || y2 <= y1) {
    return;
  }
  pixels::fill(
      pixels::Target{m_pixels.data() + rowOffset(y1, m_width) + x1, m_width},
      x2 - x1, y2 - y1, color);
}

void IndexedSurface::copy(const IndexedSurface &source, int x1, int y1, int x2,
                          int y2, int x, int y) {
  m_revision = systems::graphics::newRevision();
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

  const pixels::Source from{source.m_pixels.data() +
                                rowOffset(y1, source.m_width) + x1,
                            source.m_width};
  const pixels::Target to{m_pixels.data() + rowOffset(y, m_width) + x, m_width};
  if (&source == this) {
    pixels::move(from, to, width, height);
  } else {
    pixels::copy(from, to, width, height);
  }
}

void IndexedSurface::unpack(const Picture &picture, int x, int y) {
  m_revision = systems::graphics::newRevision();
  if (x < 0 || y < 0) {
    throw std::out_of_range("Unpack: the picture does not fit the screen");
  }
  x -= x % 8;
  if (x + picture.width > m_width || y + picture.height > m_height) {
    throw std::out_of_range("Unpack: the picture does not fit the screen");
  }
  pixels::copy(
      pixels::Source{picture.pixels.data(), picture.width},
      pixels::Target{m_pixels.data() + rowOffset(y, m_width) + x, m_width},
      picture.width, picture.height);
}

bool IndexedSurface::intersects(int left, int top, int width,
                                int height) const {
  return left < m_width && top < m_height && left + width > 0 &&
         top + height > 0;
}

void IndexedSurface::draw(const Picture &picture, int left, int top, bool flipX,
                          bool flipY, bool opaque) {
  m_revision = systems::graphics::newRevision();
  const int firstColumn = std::max(0, -left);
  const int lastColumn = std::min(picture.width, m_width - left);
  const int firstRow = std::max(0, -top);
  const int lastRow = std::min(picture.height, m_height - top);
  if (firstColumn >= lastColumn || firstRow >= lastRow) {
    return;
  }
  const int sourceRow = flipY ? picture.height - 1 - firstRow : firstRow;
  const int sourceColumn = flipX ? picture.width - lastColumn : firstColumn;
  const pixels::Source from{picture.pixels.data() +
                                rowOffset(sourceRow, picture.width) +
                                sourceColumn,
                            flipY ? -picture.width : picture.width};
  const pixels::Target to{m_pixels.data() + rowOffset(top + firstRow, m_width) +
                              left + firstColumn,
                          m_width};
  pixels::draw(from, to, lastColumn - firstColumn, lastRow - firstRow, !opaque,
               flipX);
}

void IndexedSurface::draw(const uint8_t *pixels, int width, int height,
                          int left, int top, bool opaque) {
  m_revision = systems::graphics::newRevision();
  const int firstColumn = std::max(0, -left);
  const int lastColumn = std::min(width, m_width - left);
  const int firstRow = std::max(0, -top);
  const int lastRow = std::min(height, m_height - top);
  if (firstColumn >= lastColumn || firstRow >= lastRow) {
    return;
  }
  const pixels::Source from{pixels + rowOffset(firstRow, width) + firstColumn,
                            width};
  const pixels::Target to{m_pixels.data() + rowOffset(top + firstRow, m_width) +
                              left + firstColumn,
                          m_width};
  pixels::draw(from, to, lastColumn - firstColumn, lastRow - firstRow, !opaque,
               false);
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
