#include "graphics/IndexedRasterizer.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace openfranko::src::systems::graphics {
namespace {

constexpr std::size_t AMIGA_COLORS = 4096;
constexpr uint16_t COLOR_BITS = 0xFFF;
constexpr uint16_t BLACK = 0x000;
constexpr uint32_t BYTE_LANES = 0x01010101u;
constexpr int16_t NO_SLOT = -1;
constexpr std::size_t UNROLLED_BYTES = 16;
constexpr int NO_LAYER = -1;

std::size_t blockSize(const Layer &layer) {
  const std::size_t colors =
      std::clamp<std::size_t>(layer.palette.size(), 1, FRAME_COLORS);
  const std::size_t span =
      std::min<std::size_t>(colors, static_cast<std::size_t>(layer.mask) + 1);
  std::size_t size = 1;
  while (size < span) {
    size <<= 1;
  }
  return size;
}

uint16_t layerColor(const Layer &layer, std::size_t index) {
  return index < layer.palette.size()
             ? static_cast<uint16_t>(layer.palette[index] & COLOR_BITS)
             : BLACK;
}

int sourceRowOf(const Layer &layer, int row) {
  const int line = row - layer.top;
  return layer.sourceY +
         (layer.repeat > 1 ? line / layer.repeat : line) * layer.sourceStep;
}

bool coversColumns(const Display &display, const Layer &layer) {
  if (layer.left > 0 || layer.left + layer.columns < display.width) {
    return false;
  }
  const int firstColumn = layer.sourceX - layer.left;
  return !layer.pixels || layer.wrap ||
         (firstColumn >= 0 &&
          firstColumn + display.width <= layer.sourceColumns);
}

template <typename Visit>
void forEachValue(uint8_t mask, uint8_t index, Visit visit) {
  if ((index & mask) != index) {
    return;
  }
  const std::size_t period = static_cast<std::size_t>(mask) + 1;
  if ((period & mask) == 0) {
    for (std::size_t value = index; value < FRAME_COLORS; value += period) {
      visit(value);
    }
    return;
  }
  for (std::size_t value = 0; value < FRAME_COLORS; ++value) {
    if ((value & mask) == index) {
      visit(value);
    }
  }
}

int channelDistance(uint16_t left, uint16_t right, int shift) {
  const int difference = ((left >> shift) & 0xF) - ((right >> shift) & 0xF);
  return difference * difference;
}

int colorDistance(uint16_t left, uint16_t right) {
  return channelDistance(left, right, 8) + channelDistance(left, right, 4) +
         channelDistance(left, right, 0);
}

template <bool Checked, bool Based>
uint32_t copyLanes(uint8_t *out, const uint8_t *in, std::size_t count,
                   uint32_t keep, uint32_t base) {
  uint32_t seen = 0;
  const auto copyWord = [&](std::size_t at) {
    uint32_t value = 0;
    std::memcpy(&value, in + at, sizeof(value));
    if (Checked) {
      seen |= value;
    }
    value &= keep;
    if (Based) {
      value |= base;
    }
    std::memcpy(out + at, &value, sizeof(value));
  };
  std::size_t at = 0;
  for (; at + UNROLLED_BYTES <= count; at += UNROLLED_BYTES) {
    copyWord(at);
    copyWord(at + sizeof(uint32_t));
    copyWord(at + 2 * sizeof(uint32_t));
    copyWord(at + 3 * sizeof(uint32_t));
  }
  for (; at + sizeof(uint32_t) <= count; at += sizeof(uint32_t)) {
    copyWord(at);
  }
  for (; at < count; ++at) {
    if (Checked) {
      seen |= in[at];
    }
    out[at] = static_cast<uint8_t>((in[at] & keep) | (Based ? base : 0));
  }
  return seen;
}

uint8_t copyMasked(uint8_t *out, const uint8_t *in, std::size_t count,
                   uint8_t keep, uint8_t base, bool checked) {
  if (keep == 0xFF && base == 0) {
    std::memcpy(out, in, count);
    return 0;
  }
  const uint32_t keepLanes = keep * BYTE_LANES;
  const uint32_t baseLanes = base * BYTE_LANES;
  uint32_t seen = 0;
  if (checked) {
    seen = base != 0
               ? copyLanes<true, true>(out, in, count, keepLanes, baseLanes)
               : copyLanes<true, false>(out, in, count, keepLanes, baseLanes);
  } else if (base != 0) {
    copyLanes<false, true>(out, in, count, keepLanes, baseLanes);
  } else {
    copyLanes<false, false>(out, in, count, keepLanes, baseLanes);
  }
  return static_cast<uint8_t>(seen | seen >> 8 | seen >> 16 | seen >> 24);
}

void copyMapped(uint8_t *out, const uint8_t *in, std::size_t count,
                const std::array<uint8_t, FRAME_COLORS> &slots) {
  for (std::size_t at = 0; at < count; ++at) {
    out[at] = slots[in[at]];
  }
}

} // namespace

bool equalBytes(const uint8_t *left, const uint8_t *right, std::size_t count) {
  return std::memcmp(left, right, count) == 0;
}

IndexedRasterizer::IndexedRasterizer(Compare compare) : m_compare(compare) {}

void IndexedRasterizer::rasterize(const Display &display, IndexedFrame &frame) {
  m_frame = &frame;
  const int width = std::max(display.width, 0);
  const int height = std::max(display.height, 0);
  const std::size_t size = static_cast<std::size_t>(width) * height;
  const bool resized = &frame != m_lastFrame || frame.width != width ||
                       frame.height != height || frame.pixels.size() != size;
  m_lastFrame = &frame;
  frame.width = width;
  frame.height = height;
  frame.pixels.resize(size);
  frame.changedRows.assign(static_cast<std::size_t>(height), false);
  frame.palette.fill(BLACK);
  if (m_colorSlots.size() != AMIGA_COLORS) {
    m_colorSlots.assign(AMIGA_COLORS, NO_SLOT);
  }
  for (const uint16_t color : m_assignedColors) {
    m_colorSlots[color] = NO_SLOT;
  }
  m_assignedColors.clear();
  m_used.fill(false);
  m_nextFree = 0;
  const std::size_t layers = display.layers.size();
  m_mappings.assign(layers, Mapping{});
  for (std::size_t block = FRAME_COLORS; block > 0; block /= 2) {
    for (std::size_t index = 0; index < layers; ++index) {
      if (blockSize(display.layers[index]) == block) {
        placeBlock(display.layers[index], m_mappings[index]);
      }
    }
  }
  const uint8_t border = slot(display.border);
  m_drawn.resize(layers);
  for (std::size_t index = 0; index < layers; ++index) {
    m_drawn[index] = drawn(display.layers[index], m_mappings[index]);
  }
  place(display);

  const auto same = [](const Drawn &left, const Drawn &right) {
    return (left.pixels == nullptr) == (right.pixels == nullptr) &&
           left.stride == right.stride &&
           left.sourceColumns == right.sourceColumns &&
           left.sourceRows == right.sourceRows &&
           left.sourceX == right.sourceX && left.sourceY == right.sourceY &&
           left.sourceStep == right.sourceStep && left.repeat == right.repeat &&
           left.wrap == right.wrap && left.left == right.left &&
           left.top == right.top && left.columns == right.columns &&
           left.rows == right.rows && left.mask == right.mask &&
           left.block == right.block && left.checked == right.checked &&
           left.keep == right.keep && left.base == right.base &&
           (left.block || left.slots == right.slots);
  };
  const bool redraw =
      resized || border != m_lastBorder || m_lastDrawn.size() != layers ||
      !std::equal(m_drawn.begin(), m_drawn.end(), m_lastDrawn.begin(), same);
  if (redraw) {
    m_saved.resize(layers);
    for (std::vector<uint8_t> &saved : m_saved) {
      saved.resize(size);
    }
  }
  for (int row = 0; row < height; ++row) {
    if (redraw || hasChanged(display, row)) {
      drawRow(display, row, border);
      frame.changedRows[static_cast<std::size_t>(row)] = true;
    }
  }
  m_lastDrawn.swap(m_drawn);
  m_lastBorder = border;
}

void IndexedRasterizer::placeBlock(const Layer &layer, Mapping &mapping) {
  if (!layer.pixels) {
    return;
  }
  const std::size_t size = blockSize(layer);
  for (std::size_t base = 0; base + size <= FRAME_COLORS; base += size) {
    const auto first = m_used.begin() + static_cast<std::ptrdiff_t>(base);
    if (std::any_of(first, first + static_cast<std::ptrdiff_t>(size),
                    [](bool used) { return used; })) {
      continue;
    }
    for (std::size_t index = 0; index < size; ++index) {
      const uint16_t color = layerColor(layer, index);
      m_used[base + index] = true;
      m_frame->palette[base + index] = color;
      if (m_colorSlots[color] == NO_SLOT) {
        remember(color, static_cast<uint8_t>(base + index));
      }
    }
    mapping.block = true;
    mapping.base = static_cast<uint8_t>(base);
    mapping.keep = static_cast<uint8_t>(layer.mask & (size - 1));
    mapping.checked = (layer.mask & ~(size - 1)) != 0;
    return;
  }
}

IndexedRasterizer::Mapping &IndexedRasterizer::mapped(const Layer &layer,
                                                      Mapping &mapping) {
  if (mapping.mapped) {
    return mapping;
  }
  const std::size_t size = mapping.block ? blockSize(layer) : 0;
  for (std::size_t value = 0; value < FRAME_COLORS; ++value) {
    const std::size_t index = value & layer.mask;
    mapping.slots[value] = index < size
                               ? static_cast<uint8_t>(mapping.base + index)
                               : slot(layerColor(layer, index));
  }
  mapping.mapped = true;
  return mapping;
}

uint8_t IndexedRasterizer::slot(uint16_t color) {
  color &= COLOR_BITS;
  if (m_colorSlots[color] != NO_SLOT) {
    return static_cast<uint8_t>(m_colorSlots[color]);
  }
  while (m_nextFree < FRAME_COLORS && m_used[m_nextFree]) {
    ++m_nextFree;
  }
  if (m_nextFree == FRAME_COLORS) {
    const uint8_t nearest = nearestSlot(color);
    remember(color, nearest);
    return nearest;
  }
  m_used[m_nextFree] = true;
  m_frame->palette[m_nextFree] = color;
  remember(color, static_cast<uint8_t>(m_nextFree));
  return static_cast<uint8_t>(m_nextFree);
}

void IndexedRasterizer::remember(uint16_t color, uint8_t slot) {
  m_colorSlots[color] = slot;
  m_assignedColors.push_back(color);
}

uint8_t IndexedRasterizer::nearestSlot(uint16_t color) const {
  std::size_t nearest = 0;
  int best = std::numeric_limits<int>::max();
  for (std::size_t index = 0; index < FRAME_COLORS; ++index) {
    const int distance = colorDistance(color, m_frame->palette[index]);
    if (distance < best) {
      best = distance;
      nearest = index;
    }
  }
  return static_cast<uint8_t>(nearest);
}

IndexedRasterizer::Drawn IndexedRasterizer::drawn(const Layer &layer,
                                                  Mapping &mapping) {
  Drawn result;
  result.pixels = layer.pixels;
  result.stride = layer.stride;
  result.sourceColumns = layer.sourceColumns;
  result.sourceRows = layer.sourceRows;
  result.sourceX = layer.sourceX;
  result.sourceY = layer.sourceY;
  result.sourceStep = layer.sourceStep;
  result.repeat = layer.repeat;
  result.wrap = layer.wrap;
  result.left = layer.left;
  result.top = layer.top;
  result.columns = layer.columns;
  result.rows = layer.rows;
  result.mask = layer.mask;
  result.block = mapping.block;
  result.checked = mapping.checked;
  result.keep = mapping.keep;
  result.base = mapping.base;
  if (!mapping.block) {
    result.slots = mapped(layer, mapping).slots;
  }
  return result;
}

void IndexedRasterizer::place(const Display &display) {
  const std::size_t layers = display.layers.size();
  const int height = m_frame->height;
  m_placed.resize(layers);
  m_recolored.resize(layers);
  m_topLayers.assign(static_cast<std::size_t>(height), NO_LAYER);
  for (std::size_t index = 0; index < layers; ++index) {
    const Layer &layer = display.layers[index];
    Placed &placed = m_placed[index];
    placed.first = std::max(0, layer.left);
    placed.last = std::max(placed.first,
                           std::min(display.width, layer.left + layer.columns));
    placed.firstRow = std::max(0, layer.top);
    placed.lastRow = placed.first < placed.last
                         ? std::min(height, layer.top + layer.rows)
                         : placed.firstRow;
    placed.shift = layer.sourceX - layer.left;
    placed.start = std::clamp(-placed.shift, placed.first, placed.last);
    placed.end = std::clamp(layer.sourceColumns - placed.shift, placed.start,
                            placed.last);
    placed.wrapEnd = std::clamp(2 * layer.sourceColumns - placed.shift,
                                placed.end, placed.last);
    std::vector<bool> &recolored = m_recolored[index];
    recolored.assign(static_cast<std::size_t>(height), false);
    for (const RowColor &change : layer.rowColors) {
      if (change.row >= 0 && change.row < height) {
        recolored[static_cast<std::size_t>(change.row)] = true;
      }
    }
    if (!coversColumns(display, layer)) {
      continue;
    }
    for (int row = placed.firstRow; row < placed.lastRow; ++row) {
      const int sourceRow = sourceRowOf(layer, row);
      if (!layer.pixels || layer.wrap ||
          (sourceRow >= 0 && sourceRow < layer.sourceRows)) {
        m_topLayers[static_cast<std::size_t>(row)] = static_cast<int>(index);
      }
    }
  }
}

bool IndexedRasterizer::isShown(std::size_t index, int row) const {
  const Placed &placed = m_placed[index];
  return row >= placed.firstRow && row < placed.lastRow &&
         m_topLayers[static_cast<std::size_t>(row)] <= static_cast<int>(index);
}

bool IndexedRasterizer::hasChanged(const Display &display, int row) const {
  const std::size_t offset =
      static_cast<std::size_t>(row) * static_cast<std::size_t>(m_frame->width);
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    if (!isShown(index, row)) {
      continue;
    }
    if (m_recolored[index][static_cast<std::size_t>(row)]) {
      return true;
    }
    const Layer &layer = display.layers[index];
    if (!layer.pixels) {
      continue;
    }
    const Placed &placed = m_placed[index];
    const int sourceRow = sourceRowOf(layer, row);
    const uint8_t *saved = m_saved[index].data() + offset;
    const auto changed = [&](int from, int to, int line, int column) {
      return from < to && line >= 0 && line < layer.sourceRows &&
             !m_compare(layer.pixels +
                            static_cast<std::ptrdiff_t>(line) * layer.stride +
                            column,
                        saved + from, static_cast<std::size_t>(to - from));
    };
    if (changed(placed.start, placed.end, sourceRow,
                placed.start + placed.shift) ||
        (layer.wrap &&
         changed(placed.end, placed.wrapEnd, sourceRow + layer.sourceStep,
                 placed.end + placed.shift - layer.sourceColumns))) {
      return true;
    }
  }
  return false;
}

void IndexedRasterizer::drawRow(const Display &display, int row,
                                uint8_t border) {
  const std::size_t offset =
      static_cast<std::size_t>(row) * static_cast<std::size_t>(m_frame->width);
  if (m_topLayers[static_cast<std::size_t>(row)] == NO_LAYER) {
    std::memset(m_frame->pixels.data() + offset, border,
                static_cast<std::size_t>(m_frame->width));
  }
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    if (!isShown(index, row)) {
      continue;
    }
    const Layer &layer = display.layers[index];
    const Placed &placed = m_placed[index];
    Mapping &mapping = m_mappings[index];
    uint8_t *saved = m_saved[index].data() + offset;
    if (m_recolored[index][static_cast<std::size_t>(row)]) {
      m_rowMapping = mapped(layer, mapping);
      m_rowMapping.block = false;
      recolor(layer, row);
      drawSpan(layer, placed, m_rowMapping, row, saved);
      continue;
    }
    const Mapping &shown = mapping.block ? mapping : mapped(layer, mapping);
    const uint8_t seen = drawSpan(layer, placed, shown, row, saved);
    if (mapping.block && mapping.checked &&
        (seen & layer.mask & ~(blockSize(layer) - 1)) != 0) {
      mapping.block = false;
      drawSpan(layer, placed, mapped(layer, mapping), row, saved);
    }
  }
}

uint8_t IndexedRasterizer::drawSpan(const Layer &layer, const Placed &placed,
                                    const Mapping &shown, int row,
                                    uint8_t *saved) {
  uint8_t *out = m_frame->pixels.data() +
                 static_cast<std::ptrdiff_t>(row) * m_frame->width;
  const uint8_t zero = shown.block ? shown.base : shown.slots[0];
  if (!layer.pixels) {
    std::memset(out + placed.first, zero,
                static_cast<std::size_t>(placed.last - placed.first));
    return 0;
  }
  const auto run = [&](int from, int to, int line, int column) -> uint8_t {
    if (from >= to) {
      return 0;
    }
    const std::size_t count = static_cast<std::size_t>(to - from);
    if (line < 0 || line >= layer.sourceRows) {
      if (layer.wrap) {
        std::memset(out + from, zero, count);
      }
      return 0;
    }
    const uint8_t *in = layer.pixels +
                        static_cast<std::ptrdiff_t>(line) * layer.stride +
                        column;
    std::memcpy(saved + from, in, count);
    if (shown.block) {
      return copyMasked(out + from, in, count, shown.keep, shown.base,
                        shown.checked);
    }
    copyMapped(out + from, in, count, shown.slots);
    return 0;
  };
  const int sourceRow = sourceRowOf(layer, row);
  uint8_t seen =
      run(placed.start, placed.end, sourceRow, placed.start + placed.shift);
  if (layer.wrap) {
    std::memset(out + placed.first, zero,
                static_cast<std::size_t>(placed.start - placed.first));
    seen |= run(placed.end, placed.wrapEnd, sourceRow + layer.sourceStep,
                placed.end + placed.shift - layer.sourceColumns);
    std::memset(out + placed.wrapEnd, zero,
                static_cast<std::size_t>(placed.last - placed.wrapEnd));
  }
  return seen;
}

void IndexedRasterizer::recolor(const Layer &layer, int row) {
  for (const RowColor &change : layer.rowColors) {
    if (change.row != row) {
      continue;
    }
    const uint8_t recolored = slot(change.color);
    forEachValue(layer.mask, change.index, [&](std::size_t value) {
      m_rowMapping.slots[value] = recolored;
    });
  }
}

} // namespace openfranko::src::systems::graphics
