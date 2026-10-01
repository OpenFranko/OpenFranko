#include "graphics/Canvas.h"

#include "graphics/PixelOps.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace openfranko::src::systems::graphics {

Canvas::Canvas(int width, int height)
    : m_width(std::max(width, 0)), m_height(std::max(height, 0)),
      m_pixels(static_cast<std::size_t>(m_width) * m_height, FILL_INDEX) {}

int Canvas::width() const { return m_width; }

int Canvas::height() const { return m_height; }

const std::vector<uint8_t> &Canvas::pixels() const { return m_pixels; }

const std::vector<uint16_t> &Canvas::palette() const { return m_palette; }

void Canvas::fill(uint16_t color) {
  prepare(true);
  m_palette[FILL_INDEX] = color;
  pixels::fill(pixels::Target{m_pixels.data(), m_width}, m_width, m_height,
               FILL_INDEX);
}

void Canvas::setPalette(const std::vector<uint16_t> &colors) {
  std::copy_n(colors.begin(), std::min<std::size_t>(colors.size(), FILL_INDEX),
              m_palette.begin());
}

void Canvas::draw(const IndexedBitmap &image, int x, int y) {
  prepare(covers(image, x, y));
  blit(image, x, y, false, false);
}

void Canvas::drawMasked(const IndexedBitmap &image, int x, int y,
                        bool flipped) {
  prepare(false);
  blit(image, x, y, true, flipped);
}

Display Canvas::output() const {
  m_shown = true;
  Layer layer;
  layer.pixels = m_pixels.data();
  layer.stride = m_width;
  layer.sourceColumns = m_width;
  layer.sourceRows = m_height;
  layer.columns = m_width;
  layer.rows = m_height;
  layer.palette = m_palette;
  Display display;
  display.width = m_width;
  display.height = m_height;
  display.displayHeight = m_height;
  display.layers.push_back(std::move(layer));
  return display;
}

bool Canvas::covers(const IndexedBitmap &image, int x, int y) const {
  const int left = x - image.hotspotX;
  const int top = y - image.hotspotY;
  return left <= 0 && top <= 0 && left + image.width >= m_width &&
         top + image.height >= m_height;
}

void Canvas::prepare(bool covered) {
  if (!m_shown) {
    return;
  }
  m_shown = false;
  m_spare.resize(m_pixels.size(), FILL_INDEX);
  std::swap(m_pixels, m_spare);
  if (!covered) {
    std::copy(m_spare.begin(), m_spare.end(), m_pixels.begin());
  }
}

void Canvas::blit(const IndexedBitmap &image, int x, int y, bool masked,
                  bool flipped) {
  const int left =
      x - (flipped ? image.width - image.hotspotX : image.hotspotX);
  const int top = y - image.hotspotY;
  const int firstColumn = std::max(0, -left);
  const int lastColumn = std::min(image.width, m_width - left);
  const int firstRow = std::max(0, -top);
  const int lastRow = std::min(image.height, m_height - top);

  if (firstColumn >= lastColumn || firstRow >= lastRow) {
    return;
  }
  const int sourceColumn = flipped ? image.width - lastColumn : firstColumn;
  const pixels::Source from{
      image.pixels.data() +
          static_cast<std::ptrdiff_t>(firstRow) * image.width + sourceColumn,
      image.width};
  const pixels::Target to{
      m_pixels.data() + static_cast<std::ptrdiff_t>(top + firstRow) * m_width +
          left + firstColumn,
      m_width};
  pixels::draw(from, to, lastColumn - firstColumn, lastRow - firstRow, masked,
               flipped);
}

} // namespace openfranko::src::systems::graphics
