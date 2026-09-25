#include "Canvas.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace openfranko::src::systems {
namespace {

constexpr uint8_t TRANSPARENT_INDEX = 0;

} // namespace

Canvas::Canvas(int width, int height)
    : m_width(std::max(width, 0)), m_height(std::max(height, 0)),
      m_pixels(static_cast<std::size_t>(m_width) * m_height, FILL_INDEX) {}

int Canvas::width() const { return m_width; }

int Canvas::height() const { return m_height; }

const std::vector<uint8_t> &Canvas::pixels() const { return m_pixels; }

const std::vector<uint16_t> &Canvas::palette() const { return m_palette; }

void Canvas::fill(uint16_t color) {
  m_palette[FILL_INDEX] = color;
  std::fill(m_pixels.begin(), m_pixels.end(), FILL_INDEX);
}

void Canvas::setPalette(const std::vector<uint16_t> &colors) {
  std::copy_n(colors.begin(), std::min<std::size_t>(colors.size(), FILL_INDEX),
              m_palette.begin());
}

void Canvas::draw(const IndexedBitmap &image, int x, int y) {
  blit(image, x, y, false, false);
}

void Canvas::drawMasked(const IndexedBitmap &image, int x, int y,
                        bool flipped) {
  blit(image, x, y, true, flipped);
}

Display Canvas::output() const {
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

void Canvas::blit(const IndexedBitmap &image, int x, int y, bool masked,
                  bool flipped) {
  const int left =
      x - (flipped ? image.width - image.hotspotX : image.hotspotX);
  const int top = y - image.hotspotY;
  const int firstColumn = std::max(0, -left);
  const int lastColumn = std::min(image.width, m_width - left);
  const int firstRow = std::max(0, -top);
  const int lastRow = std::min(image.height, m_height - top);

  for (int row = firstRow; row < lastRow; ++row) {
    const uint8_t *source =
        image.pixels.data() + static_cast<std::ptrdiff_t>(row) * image.width;
    uint8_t *target = m_pixels.data() +
                      static_cast<std::ptrdiff_t>(top + row) * m_width + left;
    for (int column = firstColumn; column < lastColumn; ++column) {
      const uint8_t index = source[flipped ? image.width - 1 - column : column];
      if (!masked || index != TRANSPARENT_INDEX) {
        target[column] = index;
      }
    }
  }
}

} // namespace openfranko::src::systems
