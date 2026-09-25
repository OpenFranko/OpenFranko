#include "Display.h"

#include <algorithm>
#include <cstddef>

namespace openfranko::src::systems {
namespace {

constexpr uint32_t OPAQUE = 0xFF000000u;
constexpr int CHANNEL_STEP = 17;

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

} // namespace openfranko::src::systems
