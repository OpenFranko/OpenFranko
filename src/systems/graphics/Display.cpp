#include "graphics/Display.h"

#include <algorithm>
#include <cstddef>

namespace openfranko::src::systems::graphics {
namespace {

constexpr uint32_t OPAQUE = 0xFF000000u;

void mapPalette(const Layer &layer, int row, std::vector<uint32_t> &colors) {
  colors.assign(256, OPAQUE);
  for (std::size_t index = 0;
       index < layer.palette.size() && index < colors.size(); ++index) {
    colors[index] = toArgb(layer.palette[index]);
  }
  for (const RowColor &change : layer.rowColors) {
    if (change.row == row) {
      colors[change.index] = toArgb(change.color);
    }
  }
}

} // namespace

RowColors::RowColors(std::initializer_list<RowColor> rows) {
  if (rows.size() != 0) {
    m_rows = std::make_shared<std::vector<RowColor>>(rows);
  }
}

bool RowColors::empty() const { return !m_rows || m_rows->empty(); }

std::size_t RowColors::size() const { return m_rows ? m_rows->size() : 0; }

const RowColor *RowColors::begin() const {
  return m_rows ? m_rows->data() : nullptr;
}

const RowColor *RowColors::end() const {
  return m_rows ? m_rows->data() + m_rows->size() : nullptr;
}

RowColor *RowColors::begin() { return m_rows ? owned().data() : nullptr; }

RowColor *RowColors::end() {
  return m_rows ? owned().data() + m_rows->size() : nullptr;
}

void RowColors::push_back(const RowColor &row) { owned().push_back(row); }

void RowColors::pop_back() { owned().pop_back(); }

void RowColors::clear() { m_rows.reset(); }

bool RowColors::shares(const RowColors &other) const {
  return m_rows == other.m_rows;
}

std::vector<RowColor> &RowColors::owned() {
  if (!m_rows) {
    m_rows = std::make_shared<std::vector<RowColor>>();
  } else if (m_rows.use_count() > 1) {
    m_rows = std::make_shared<std::vector<RowColor>>(*m_rows);
  }
  return *m_rows;
}

uint32_t newRevision() {
  static uint32_t revision = 0;
  if (++revision == 0) {
    ++revision;
  }
  return revision;
}

Layer solidLayer(uint16_t color, int top, int rows, int columns) {
  Layer layer;
  layer.top = top;
  layer.rows = rows;
  layer.columns = columns;
  layer.palette = {color};
  return layer;
}

void cropRows(Display &display, int first, int count) {
  display.height = count;
  display.displayHeight = count;
  for (Layer &layer : display.layers) {
    layer.top -= first;
  }
}

uint32_t toArgb(uint16_t color) {
  const uint32_t red = ((color >> 8) & 0xF) * CHANNEL_STEP;
  const uint32_t green = ((color >> 4) & 0xF) * CHANNEL_STEP;
  const uint32_t blue = (color & 0xF) * CHANNEL_STEP;
  return OPAQUE | red << 16 | green << 8 | blue;
}

void rasterize(const Display &display, std::vector<uint32_t> &argb) {
  argb.assign(static_cast<std::size_t>(display.width) * display.height,
              toArgb(display.border));
  std::vector<uint32_t> colors;
  for (const Layer &layer : display.layers) {
    const int firstRow = std::max(0, layer.top);
    const int lastRow = std::min(display.height, layer.top + layer.rows);
    const int firstColumn = std::max(0, layer.left);
    const int lastColumn = std::min(display.width, layer.left + layer.columns);
    if (firstColumn >= lastColumn) {
      continue;
    }
    mapPalette(layer, -1, colors);
    for (int row = firstRow; row < lastRow; ++row) {
      if (!layer.rowColors.empty()) {
        mapPalette(layer, row, colors);
      }
      uint32_t *out =
          argb.data() + static_cast<std::ptrdiff_t>(row) * display.width;
      if (!layer.pixels) {
        std::fill(out + firstColumn, out + lastColumn, colors[0]);
        continue;
      }
      const int sourceRow = layer.sourceY + (row - layer.top) /
                                                std::max(layer.repeat, 1) *
                                                layer.sourceStep;
      for (int x = firstColumn; x < lastColumn; ++x) {
        int column = layer.sourceX + x - layer.left;
        int y = sourceRow;
        if (layer.wrap && column >= layer.sourceColumns) {
          column -= layer.sourceColumns;
          y += layer.sourceStep;
        }
        const bool inside = y >= 0 && y < layer.sourceRows && column >= 0 &&
                            column < layer.sourceColumns;
        if (!inside && !layer.wrap) {
          continue;
        }
        const uint8_t index =
            inside
                ? layer.pixels[static_cast<std::ptrdiff_t>(y) * layer.stride +
                               column]
                : 0;
        out[x] = colors[index & layer.mask];
      }
    }
  }
}

} // namespace openfranko::src::systems::graphics
