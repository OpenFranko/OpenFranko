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

uint8_t withSprites(const Layer &layer, int column, int row, uint8_t value) {
  for (const Sprite &sprite : layer.sprites) {
    const int x = column - sprite.left;
    const int y = row - sprite.top;
    if (x < 0 || x >= sprite.width || y < 0 || y >= sprite.height) {
      continue;
    }
    const uint8_t pixel =
        sprite.pixels[static_cast<std::ptrdiff_t>(y) * sprite.width + x];
    if (pixel != 0) {
      value = pixel;
    }
  }
  return value;
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

void assign(Display &target, const Display &source) {
  target.width = source.width;
  target.height = source.height;
  target.displayHeight = source.displayHeight;
  target.border = source.border;
  target.revision = source.revision;
  if (target.layers.size() != source.layers.size()) {
    target.layers = source.layers;
    return;
  }
  Layer *to = target.layers.data();
  for (const Layer &from : source.layers) {
    Layer &layer = *to++;
    layer.pixels = from.pixels;
    layer.stride = from.stride;
    layer.sourceColumns = from.sourceColumns;
    layer.sourceRows = from.sourceRows;
    layer.sourceX = from.sourceX;
    layer.sourceY = from.sourceY;
    layer.sourceStep = from.sourceStep;
    layer.repeat = from.repeat;
    layer.wrap = from.wrap;
    layer.left = from.left;
    layer.top = from.top;
    layer.columns = from.columns;
    layer.rows = from.rows;
    layer.mask = from.mask;
    layer.revision = from.revision;
    layer.carriesSprites = from.carriesSprites;
    if (!layer.sprites.empty() || !from.sprites.empty()) {
      layer.sprites = from.sprites;
    }
    if (!layer.palette.empty() || !from.palette.empty()) {
      layer.palette = from.palette;
    }
    if (!layer.rowColors.shares(from.rowColors)) {
      layer.rowColors = from.rowColors;
    }
  }
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
        uint8_t index =
            inside
                ? layer.pixels[static_cast<std::ptrdiff_t>(y) * layer.stride +
                               column]
                : 0;
        if (inside && !layer.sprites.empty()) {
          index = withSprites(layer, column, y, index);
        }
        out[x] = colors[index & layer.mask];
      }
    }
  }
}

} // namespace openfranko::src::systems::graphics
