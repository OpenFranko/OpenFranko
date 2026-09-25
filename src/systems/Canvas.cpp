#include "Canvas.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace openfranko::src::systems {
namespace {

constexpr uint32_t OPAQUE = 0xFF000000u;
constexpr int CHANNEL_STEP = 17;
constexpr uint8_t TRANSPARENT_INDEX = 0;

uint32_t toArgb(uint16_t color) {
  const uint32_t red = ((color >> 8) & 0xF) * CHANNEL_STEP;
  const uint32_t green = ((color >> 4) & 0xF) * CHANNEL_STEP;
  const uint32_t blue = (color & 0xF) * CHANNEL_STEP;
  return OPAQUE | red << 16 | green << 8 | blue;
}

} // namespace

Canvas::Canvas(int width, int height)
    : m_width(std::max(width, 0)), m_height(std::max(height, 0)),
      m_pixels(static_cast<std::size_t>(m_width) * m_height, OPAQUE) {}

int Canvas::width() const { return m_width; }

int Canvas::height() const { return m_height; }

const std::vector<uint32_t> &Canvas::pixels() const { return m_pixels; }

void Canvas::fill(uint16_t color) {
  std::fill(m_pixels.begin(), m_pixels.end(), toArgb(color));
}

void Canvas::draw(const IndexedBitmap &image,
                  const std::vector<uint16_t> &palette, int x, int y) {
  blit(image, palette, x, y, false, false);
}

void Canvas::drawMasked(const IndexedBitmap &image,
                        const std::vector<uint16_t> &palette, int x, int y,
                        bool flipped) {
  blit(image, palette, x, y, true, flipped);
}

void Canvas::blit(const IndexedBitmap &image,
                  const std::vector<uint16_t> &palette, int x, int y,
                  bool masked, bool flipped) {
  std::array<uint32_t, 256> colors;
  colors.fill(OPAQUE);
  for (std::size_t index = 0; index < palette.size() && index < colors.size();
       ++index) {
    colors[index] = toArgb(palette[index]);
  }

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
    uint32_t *target = m_pixels.data() +
                       static_cast<std::ptrdiff_t>(top + row) * m_width + left;
    for (int column = firstColumn; column < lastColumn; ++column) {
      const uint8_t index = source[flipped ? image.width - 1 - column : column];
      if (!masked || index != TRANSPARENT_INDEX) {
        target[column] = colors[index];
      }
    }
  }
}

} // namespace openfranko::src::systems
