#include "graphics/IndexedRasterizer.h"

#include <algorithm>
#include <cstdlib>
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
constexpr int NEW_LAYOUT = -2;
constexpr int SHIFT_ALIGNMENT = 4;

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

bool sameLayout(const Layer &left, const Layer &right) {
  return (left.pixels == nullptr) == (right.pixels == nullptr) &&
         left.stride == right.stride &&
         left.sourceColumns == right.sourceColumns &&
         left.sourceRows == right.sourceRows && left.sourceY == right.sourceY &&
         left.sourceStep == right.sourceStep && left.repeat == right.repeat &&
         left.wrap == right.wrap && left.left == right.left &&
         left.top == right.top && left.columns == right.columns &&
         left.rows == right.rows && left.mask == right.mask &&
         left.palette == right.palette &&
         (left.rowColors.shares(right.rowColors) ||
          std::equal(left.rowColors.begin(), left.rowColors.end(),
                     right.rowColors.begin(), right.rowColors.end(),
                     [](const RowColor &a, const RowColor &b) {
                       return a.row == b.row && a.index == b.index &&
                              a.color == b.color;
                     }));
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
  std::size_t at = 0;
  for (; at + sizeof(uint32_t) <= count; at += sizeof(uint32_t)) {
    out[at] = slots[in[at]];
    out[at + 1] = slots[in[at + 1]];
    out[at + 2] = slots[in[at + 2]];
    out[at + 3] = slots[in[at + 3]];
  }
  for (; at < count; ++at) {
    out[at] = slots[in[at]];
  }
}

void overlayRun(uint8_t *out, const uint8_t *in, std::size_t count,
                const std::array<uint8_t, FRAME_COLORS> &slots) {
  constexpr uint32_t ONES = 0x01010101u;
  constexpr uint32_t HIGHS = 0x80808080u;
  std::size_t at = 0;
  for (; at + sizeof(uint32_t) <= count; at += sizeof(uint32_t)) {
    uint32_t quad;
    std::memcpy(&quad, in + at, sizeof(quad));
    if (quad == 0) {
      continue;
    }
    if (((quad - ONES) & ~quad & HIGHS) == 0) {
      out[at] = slots[in[at]];
      out[at + 1] = slots[in[at + 1]];
      out[at + 2] = slots[in[at + 2]];
      out[at + 3] = slots[in[at + 3]];
      continue;
    }
    for (std::size_t pixel = at; pixel < at + sizeof(uint32_t); ++pixel) {
      if (in[pixel] != 0) {
        out[pixel] = slots[in[pixel]];
      }
    }
  }
  for (; at < count; ++at) {
    if (in[at] != 0) {
      out[at] = slots[in[at]];
    }
  }
}

void addSpan(std::array<Span, 2> &spans, std::size_t slots, Span span) {
  if (span.first >= span.last) {
    return;
  }
  for (std::size_t at = 0; at < slots; ++at) {
    if (spans[at].first >= spans[at].last) {
      spans[at] = span;
      return;
    }
  }
  std::size_t best = 0;
  int growth = std::numeric_limits<int>::max();
  for (std::size_t at = 0; at < slots; ++at) {
    const int grown = std::max(spans[at].last, span.last) -
                      std::min(spans[at].first, span.first) -
                      (spans[at].last - spans[at].first);
    if (grown < growth) {
      growth = grown;
      best = at;
    }
  }
  spans[best] = {std::min(spans[best].first, span.first),
                 std::max(spans[best].last, span.last)};
}

int floorDivide(int value, int divisor) {
  return value >= 0 ? value / divisor : -((divisor - 1 - value) / divisor);
}

int spanWidth(const RowChange &change) {
  int width = 0;
  for (const Span &span : change.spans) {
    width += std::max(0, span.last - span.first);
  }
  return width;
}

} // namespace

bool isChanged(const RowChange &change) {
  return change.from != NO_ROW || change.shift != 0 ||
         std::any_of(change.spans.begin(), change.spans.end(),
                     [](const Span &span) { return span.first < span.last; });
}

std::size_t leadingBytes(const uint8_t *left, const uint8_t *right,
                         std::size_t count) {
  return static_cast<std::size_t>(
      std::mismatch(left, left + count, right).first - left);
}

std::size_t trailingBytes(const uint8_t *left, const uint8_t *right,
                          std::size_t count) {
  std::size_t equal = 0;
  while (equal < count && left[count - 1 - equal] == right[count - 1 - equal]) {
    ++equal;
  }
  return equal;
}

IndexedRasterizer::IndexedRasterizer(Scan leading, Scan trailing)
    : m_leading(leading), m_trailing(trailing) {}

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
  frame.changes.assign(static_cast<std::size_t>(height), RowChange{});
  m_streak = 0;
  m_sourceKept.assign(display.layers.size(), false);
  m_anyKept = false;
  m_anySprites =
      std::any_of(display.layers.begin(), display.layers.end(),
                  [](const Layer &layer) { return !layer.sprites.empty(); });
  for (std::size_t index = 0;
       !resized &&
       index < std::min(display.layers.size(), m_lastDisplay.layers.size());
       ++index) {
    const uint32_t revision = display.layers[index].revision;
    m_sourceKept[index] =
        revision != 0 && revision == m_lastDisplay.layers[index].revision;
    m_anyKept = m_anyKept || m_sourceKept[index];
  }
  const int panned = resized ? NEW_LAYOUT : pannedLayer(display);
  if (panned != NEW_LAYOUT) {
    for (Mapping &mapping : m_mappings) {
      mapping.block = mapping.placed;
    }
    m_colorsKept = true;
    m_updates.assign(static_cast<std::size_t>(height), Update::None);
    if (panned != NO_LAYER) {
      panLayer(display, static_cast<std::size_t>(panned));
    }
    findChanges(display);
    markSprites(display);
  } else {
    arrange(display, resized);
  }
  int row = 0;
  while (row < height) {
    const Update update = m_updates[static_cast<std::size_t>(row)];
    if (update == Update::Full) {
      drawRow(display, row, m_border);
      frame.changes[static_cast<std::size_t>(row)].spans[0] = {0, width};
    }
    if (update != Update::Spans && update != Update::Panned) {
      ++row;
      continue;
    }
    const int shift = frame.changes[static_cast<std::size_t>(row)].shift;
    int lastRow = row + 1;
    while (shift != 0 && lastRow < height &&
           m_updates[static_cast<std::size_t>(lastRow)] == update &&
           frame.changes[static_cast<std::size_t>(lastRow)].shift == shift) {
      ++lastRow;
    }
    shiftRows(row, lastRow, shift);
    for (; row < lastRow; ++row) {
      drawSpans(display, row);
    }
  }
  drawExposed(display);
  for (row = 0; row < height; ++row) {
    const Update update = m_updates[static_cast<std::size_t>(row)];
    if (update == Update::None) {
      continue;
    }
    const int only = m_onlyLayers[static_cast<std::size_t>(row)];
    if (m_anyKept && (update == Update::Spans || update == Update::Panned) &&
        m_sourceKept[static_cast<std::size_t>(only)]) {
      continue;
    }
    if ((update == Update::Spans || update == Update::Panned) &&
        !isPlain(display, static_cast<std::size_t>(only), row)) {
      const std::array<Span, 2> &spans =
          frame.changes[static_cast<std::size_t>(row)].spans;
      for (std::size_t at = 0; at < (update == Update::Panned ? 1 : 2); ++at) {
        if (spans[at].first < spans[at].last) {
          saveWindow(display, static_cast<std::size_t>(only), row,
                     spans[at].first, spans[at].last);
        }
      }
    } else {
      save(display, row);
    }
  }
  m_lastDisplay = display;
}

int IndexedRasterizer::pannedLayer(const Display &display) const {
  if (display.width != m_lastDisplay.width ||
      display.height != m_lastDisplay.height ||
      display.border != m_lastDisplay.border ||
      display.layers.size() != m_lastDisplay.layers.size()) {
    return NEW_LAYOUT;
  }
  int panned = NO_LAYER;
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    const Layer &now = display.layers[index];
    const Layer &before = m_lastDisplay.layers[index];
    if (!sameLayout(now, before)) {
      return NEW_LAYOUT;
    }
    if (now.sourceX == before.sourceX) {
      continue;
    }
    if (panned != NO_LAYER ||
        std::abs(now.sourceX - before.sourceX) >= display.width ||
        coversColumns(display, now) != coversColumns(m_lastDisplay, before)) {
      return NEW_LAYOUT;
    }
    panned = static_cast<int>(index);
  }
  return panned;
}

void IndexedRasterizer::panLayer(const Display &display, std::size_t index) {
  const Layer &layer = display.layers[index];
  const int moved = layer.sourceX - m_lastDisplay.layers[index].sourceX;
  Placed &placed = m_placed[index];
  placed.shift = layer.sourceX - layer.left;
  placed.start = std::clamp(-placed.shift, placed.first, placed.last);
  placed.end =
      std::clamp(layer.sourceColumns - placed.shift, placed.start, placed.last);
  placed.wrapEnd = std::clamp(2 * layer.sourceColumns - placed.shift,
                              placed.end, placed.last);
  m_lastDrawn[index].sourceX = layer.sourceX;
  panRows(display, index, moved);
}

void IndexedRasterizer::arrange(const Display &display, bool resized) {
  const int height = m_frame->height;
  m_frame->palette.fill(BLACK);
  if (m_colorSlots.size() != AMIGA_COLORS) {
    m_colorSlots.assign(AMIGA_COLORS, NO_SLOT);
  }
  for (const uint16_t color : m_assignedColors) {
    m_colorSlots[color] = NO_SLOT;
  }
  m_assignedColors.clear();
  if (m_recolorSlots.size() != AMIGA_COLORS) {
    m_recolorSlots.assign(AMIGA_COLORS, NO_SLOT);
  }
  for (const uint16_t color : m_recolorColors) {
    m_recolorSlots[color] = NO_SLOT;
  }
  m_recolorColors.clear();
  m_used.fill(false);
  m_nextFree = 0;
  const std::size_t layers = display.layers.size();
  m_mappings.assign(layers, Mapping{});
  m_rowLayer = NO_LAYER;
  for (std::size_t block = FRAME_COLORS; block > 0; block /= 2) {
    for (std::size_t index = 0; index < layers; ++index) {
      if (blockSize(display.layers[index]) == block) {
        placeBlock(display.layers[index], m_mappings[index]);
      }
    }
  }
  m_colorsKept = !resized && keepsColors(display);
  m_recoloredRows.assign(static_cast<std::size_t>(height), false);
  if (!resized && !m_colorsKept) {
    for (const std::vector<int> &starts : m_recolorStarts) {
      for (std::size_t row = 0; !starts.empty() && row + 1 < starts.size();
           ++row) {
        if (starts[row + 1] > starts[row]) {
          m_recoloredRows[row] = true;
        }
      }
    }
  }
  place(display);
  assignRecolors(display);
  m_border = slot(display.border);
  m_drawn.resize(layers);
  for (std::size_t index = 0; index < layers; ++index) {
    m_drawn[index] = drawn(display.layers[index], m_mappings[index]);
  }

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
           (left.block || (left.pixels ? left.slots == right.slots
                                       : left.slots[0] == right.slots[0]));
  };
  bool redraw =
      resized || m_border != m_lastBorder || m_lastDrawn.size() != layers;
  int movedLayer = NO_LAYER;
  int pannedLayer = NO_LAYER;
  int panned = 0;
  for (std::size_t index = 0; !redraw && index < layers; ++index) {
    const Drawn &now = m_drawn[index];
    const Drawn &before = m_lastDrawn[index];
    if (same(now, before)) {
      continue;
    }
    const bool first = movedLayer == NO_LAYER && pannedLayer == NO_LAYER;
    Drawn moved = now;
    moved.sourceY = before.sourceY;
    Drawn shifted = now;
    shifted.sourceX = before.sourceX;
    if (first && same(moved, before) && now.repeat <= 1 &&
        now.sourceStep == 1) {
      movedLayer = static_cast<int>(index);
    } else if (first && m_colorsKept && same(shifted, before) &&
               std::abs(now.sourceX - before.sourceX) < m_frame->width) {
      pannedLayer = static_cast<int>(index);
      panned = now.sourceX - before.sourceX;
    } else {
      redraw = true;
    }
  }
  if (redraw) {
    m_saved.resize(layers);
    for (std::size_t index = 0; index < layers; ++index) {
      const Layer &layer = display.layers[index];
      m_saved[index].resize(
          layer.pixels
              ? static_cast<std::size_t>(std::max(layer.sourceRows, 0)) *
                        static_cast<std::size_t>(std::max(layer.stride, 0)) +
                    static_cast<std::size_t>(std::max(layer.sourceColumns, 0))
              : 0);
    }
    m_updates.assign(static_cast<std::size_t>(height), Update::Full);
  } else {
    m_updates.assign(static_cast<std::size_t>(height), Update::None);
    for (int row = 0; !m_colorsKept && row < height; ++row) {
      if (m_recoloredRows[static_cast<std::size_t>(row)]) {
        m_updates[static_cast<std::size_t>(row)] = Update::Full;
      }
    }
    if (movedLayer != NO_LAYER) {
      moveRows(static_cast<std::size_t>(movedLayer));
    }
    if (pannedLayer != NO_LAYER) {
      panRows(display, static_cast<std::size_t>(pannedLayer), panned);
    }
    findChanges(display);
    markSprites(display);
  }
  m_lastDrawn.swap(m_drawn);
  m_lastSoleLayers = m_soleLayers;
  m_lastBorder = m_border;
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
    mapping.placed = true;
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
  mapping.mapped = true;
  if (!layer.pixels) {
    mapping.slots[0] = slot(layerColor(layer, 0));
    return mapping;
  }
  const std::size_t size = mapping.placed ? blockSize(layer) : 0;
  for (std::size_t value = 0; value < FRAME_COLORS; ++value) {
    const std::size_t index = value & layer.mask;
    mapping.slots[value] = index < size
                               ? static_cast<uint8_t>(mapping.base + index)
                               : slot(layerColor(layer, index));
  }
  return mapping;
}

uint8_t IndexedRasterizer::recolorSlot(uint16_t color) {
  color &= COLOR_BITS;
  if (m_recolorSlots[color] != NO_SLOT) {
    return static_cast<uint8_t>(m_recolorSlots[color]);
  }
  while (m_nextFree < FRAME_COLORS && m_used[m_nextFree]) {
    ++m_nextFree;
  }
  uint8_t chosen = 0;
  if (m_nextFree == FRAME_COLORS) {
    chosen = nearestSlot(color);
  } else {
    chosen = static_cast<uint8_t>(m_nextFree);
    m_used[m_nextFree] = true;
    m_frame->palette[m_nextFree] = color;
  }
  m_recolorSlots[color] = chosen;
  m_recolorColors.push_back(color);
  if (m_colorSlots[color] == NO_SLOT) {
    remember(color, chosen);
  }
  return chosen;
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

void IndexedRasterizer::assignRecolors(const Display &display) {
  const std::size_t layers = display.layers.size();
  m_recolorTargets.resize(layers);
  bool recolors = false;
  for (std::size_t index = 0; index < layers; ++index) {
    m_recolorTargets[index].assign(m_recolorOrder[index].size(), 0);
    recolors = recolors || !m_recolorOrder[index].empty();
  }
  if (!recolors) {
    return;
  }
  const int height = m_frame->height;
  for (int row = 0; row < height; ++row) {
    for (std::size_t index = 0; index < layers; ++index) {
      if (!isRecolored(index, row) || !isShown(index, row)) {
        continue;
      }
      const std::vector<int> &starts = m_recolorStarts[index];
      const std::vector<int> &order = m_recolorOrder[index];
      const RowColor *changes = display.layers[index].rowColors.begin();
      for (int at = starts[static_cast<std::size_t>(row)];
           at < starts[static_cast<std::size_t>(row) + 1]; ++at) {
        m_recolorTargets[index][static_cast<std::size_t>(at)] =
            recolorSlot(changes[order[static_cast<std::size_t>(at)]].color);
      }
    }
  }
}

bool IndexedRasterizer::keepsColors(const Display &display) const {
  return std::equal(
      display.layers.begin(), display.layers.end(),
      m_lastDisplay.layers.begin(), m_lastDisplay.layers.end(),
      [](const Layer &left, const Layer &right) {
        return left.rowColors.shares(right.rowColors) ||
               std::equal(left.rowColors.begin(), left.rowColors.end(),
                          right.rowColors.begin(), right.rowColors.end(),
                          [](const RowColor &a, const RowColor &b) {
                            return a.row == b.row && a.index == b.index &&
                                   a.color == b.color;
                          });
      });
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
  } else if (mapping.checked) {
    slot(BLACK);
  }
  return result;
}

void IndexedRasterizer::place(const Display &display) {
  const std::size_t layers = display.layers.size();
  const int height = m_frame->height;
  m_placed.resize(layers);
  m_recolorStarts.resize(layers);
  m_recolorOrder.resize(layers);
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
    std::vector<int> &starts = m_recolorStarts[index];
    std::vector<int> &order = m_recolorOrder[index];
    starts.clear();
    order.clear();
    if (!layer.rowColors.empty()) {
      starts.assign(static_cast<std::size_t>(height) + 1, 0);
      for (const RowColor &change : layer.rowColors) {
        if (change.row >= 0 && change.row < height) {
          ++starts[static_cast<std::size_t>(change.row) + 1];
        }
      }
      for (std::size_t row = 0; row < static_cast<std::size_t>(height); ++row) {
        starts[row + 1] += starts[row];
      }
      order.resize(static_cast<std::size_t>(starts.back()));
      m_recolorNext.assign(starts.begin(), starts.end() - 1);
      int position = 0;
      for (const RowColor &change : layer.rowColors) {
        if (change.row >= 0 && change.row < height) {
          order[static_cast<std::size_t>(
              m_recolorNext[static_cast<std::size_t>(change.row)]++)] =
              position;
        }
        ++position;
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
  m_soleLayers.assign(static_cast<std::size_t>(height), 0);
  for (std::size_t index = 0; index < layers; ++index) {
    const Placed &placed = m_placed[index];
    for (int row = placed.firstRow; row < placed.lastRow; ++row) {
      if (m_topLayers[static_cast<std::size_t>(row)] <=
          static_cast<int>(index)) {
        ++m_soleLayers[static_cast<std::size_t>(row)];
      }
    }
  }
  m_onlyLayers.assign(static_cast<std::size_t>(height), NO_LAYER);
  for (int row = 0; row < height; ++row) {
    const int top = m_topLayers[static_cast<std::size_t>(row)];
    int &sole = m_soleLayers[static_cast<std::size_t>(row)];
    const bool only = sole == 1 && top != NO_LAYER;
    m_onlyLayers[static_cast<std::size_t>(row)] = only ? top : NO_LAYER;
    sole = only && isPlain(display, static_cast<std::size_t>(top), row)
               ? top
               : NO_LAYER;
  }
}

bool IndexedRasterizer::isShown(std::size_t index, int row) const {
  const Placed &placed = m_placed[index];
  return row >= placed.firstRow && row < placed.lastRow &&
         m_topLayers[static_cast<std::size_t>(row)] <= static_cast<int>(index);
}

bool IndexedRasterizer::isPlain(const Display &display, std::size_t index,
                                int row) const {
  const Layer &layer = display.layers[index];
  return layer.pixels && !layer.wrap && !isRecolored(index, row);
}

bool IndexedRasterizer::isRecolored(std::size_t index, int row) const {
  const std::vector<int> &starts = m_recolorStarts[index];
  return !starts.empty() && starts[static_cast<std::size_t>(row) + 1] >
                                starts[static_cast<std::size_t>(row)];
}

void IndexedRasterizer::findChanges(const Display &display) {
  const int height = m_frame->height;
  int row = 0;
  while (row < height) {
    const int sole = m_soleLayers[static_cast<std::size_t>(row)];
    if (m_updates[static_cast<std::size_t>(row)] != Update::None) {
      ++row;
      continue;
    }
    if (sole == NO_LAYER) {
      const int only = m_onlyLayers[static_cast<std::size_t>(row)];
      if (only != NO_LAYER) {
        compareWindow(display, static_cast<std::size_t>(only), row);
      } else if (hasChanged(display, row)) {
        m_updates[static_cast<std::size_t>(row)] = Update::Full;
      }
      ++row;
      continue;
    }
    int lastRow = row + 1;
    while (lastRow < height &&
           m_soleLayers[static_cast<std::size_t>(lastRow)] == sole &&
           m_updates[static_cast<std::size_t>(lastRow)] == Update::None) {
      ++lastRow;
    }
    compareRows(display, static_cast<std::size_t>(sole), row, lastRow);
    row = lastRow;
  }
}

void IndexedRasterizer::moveRows(std::size_t index) {
  const Placed &placed = m_placed[index];
  const int height = m_frame->height;
  const int moved = m_drawn[index].sourceY - m_lastDrawn[index].sourceY;
  const bool upward = moved > 0;
  const int count = placed.lastRow - placed.firstRow;
  for (int step = 0; step < count; ++step) {
    const int row = upward ? placed.firstRow + step : placed.lastRow - 1 - step;
    if (m_topLayers[static_cast<std::size_t>(row)] > static_cast<int>(index)) {
      continue;
    }
    const int from = row + moved;
    const bool kept = m_soleLayers[static_cast<std::size_t>(row)] ==
                          static_cast<int>(index) &&
                      from >= 0 && from < height &&
                      m_lastSoleLayers[static_cast<std::size_t>(from)] ==
                          static_cast<int>(index);
    if (!kept) {
      m_updates[static_cast<std::size_t>(row)] = Update::Full;
      continue;
    }
    std::memmove(m_frame->pixels.data() +
                     static_cast<std::ptrdiff_t>(row) * m_frame->width,
                 m_frame->pixels.data() +
                     static_cast<std::ptrdiff_t>(from) * m_frame->width,
                 static_cast<std::size_t>(m_frame->width));
    m_frame->changes[static_cast<std::size_t>(row)].from = from;
  }
}

void IndexedRasterizer::panRows(const Display &display, std::size_t index,
                                int moved) {
  const int height = m_frame->height;
  const int width = m_frame->width;
  const Span exposed = moved > 0 ? Span{width - moved, width} : Span{0, -moved};
  const Span kept = moved > 0 ? Span{0, width - moved} : Span{-moved, width};
  for (int row = 0; row < height; ++row) {
    const std::size_t at = static_cast<std::size_t>(row);
    if (m_onlyLayers[at] == static_cast<int>(index)) {
      RowChange &change = m_frame->changes[at];
      change.shift = -moved;
      change.spans = {changedSpan(display, index, row, kept.first, kept.last),
                      exposed};
      m_updates[at] = Update::Panned;
      m_exposedLayer = static_cast<int>(index);
      m_exposed = exposed;
    } else if (isShown(index, row)) {
      m_updates[at] = Update::Full;
    }
  }
}

void IndexedRasterizer::compareWindow(const Display &display, std::size_t index,
                                      int row) {
  if (!m_colorsKept && isRecolored(index, row)) {
    m_updates[static_cast<std::size_t>(row)] = Update::Full;
    return;
  }
  const Span changed = changedSpan(display, index, row, 0, m_frame->width);
  if (changed.first < changed.last) {
    m_frame->changes[static_cast<std::size_t>(row)].spans[0] = changed;
    m_updates[static_cast<std::size_t>(row)] = Update::Spans;
  }
}

Span IndexedRasterizer::changedSpan(const Display &display, std::size_t index,
                                    int row, int from, int to) const {
  const Layer &layer = display.layers[index];
  Span changed{to, from};
  if (!layer.pixels || m_sourceKept[index]) {
    return {};
  }
  const Placed &placed = m_placed[index];
  const int sourceRow = sourceRowOf(layer, row);
  const auto scan = [&](int first, int last, int line, int column) {
    const int low = std::max(first, from);
    const int high = std::min(last, to);
    if (low >= high || line < 0 || line >= layer.sourceRows) {
      return;
    }
    const std::size_t offset = static_cast<std::size_t>(line) *
                                   static_cast<std::size_t>(layer.stride) +
                               static_cast<std::size_t>(column + low - first);
    const uint8_t *in = layer.pixels + offset;
    const uint8_t *saved = m_saved[index].data() + offset;
    const std::size_t count = static_cast<std::size_t>(high - low);
    const std::size_t lead = m_leading(in, saved, count);
    if (lead == count) {
      return;
    }
    const std::size_t trail = m_trailing(in + lead, saved + lead, count - lead);
    changed.first = std::min(changed.first, low + static_cast<int>(lead));
    changed.last = std::max(changed.last, high - static_cast<int>(trail));
  };
  scan(placed.start, placed.end, sourceRow, placed.start + placed.shift);
  if (layer.wrap) {
    scan(placed.end, placed.wrapEnd, sourceRow + layer.sourceStep,
         placed.end + placed.shift - layer.sourceColumns);
  }
  return changed.first < changed.last ? changed : Span{};
}

void IndexedRasterizer::markSprites(const Display &display) {
  static const std::vector<Sprite> none;
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    const Layer &layer = display.layers[index];
    const std::vector<Sprite> &before =
        index < m_lastDisplay.layers.size()
            ? m_lastDisplay.layers[index].sprites
            : none;
    if (!layer.pixels || (layer.sprites.empty() && before.empty())) {
      continue;
    }
    for (std::size_t at = 0; at < before.size(); ++at) {
      markSprite(display, index, before[at],
                 at < layer.sprites.size() && layer.sprites[at] == before[at],
                 true);
    }
    for (std::size_t at = 0; at < layer.sprites.size(); ++at) {
      markSprite(display, index, layer.sprites[at],
                 at < before.size() && layer.sprites[at] == before[at], false);
    }
  }
}

void IndexedRasterizer::markSprite(const Display &display, std::size_t index,
                                   const Sprite &sprite, bool kept, bool old) {
  const Layer &layer = display.layers[index];
  const Placed &placed = m_placed[index];
  const Span shown = spriteColumns(layer, placed, sprite, false);
  const Span wrapped =
      layer.wrap ? spriteColumns(layer, placed, sprite, true) : Span{};
  if (shown.first >= shown.last && wrapped.first >= wrapped.last) {
    return;
  }
  int firstRow = placed.firstRow;
  int lastRow = placed.lastRow;
  if (layer.sourceStep > 0 && layer.repeat > 0) {
    const int lowest =
        std::max(-floorDivide(layer.sourceY - sprite.top, layer.sourceStep) -
                     (layer.wrap ? 1 : 0),
                 0);
    const int highest = floorDivide(
        sprite.top + sprite.height - 1 - layer.sourceY, layer.sourceStep);
    firstRow = std::max(firstRow, layer.top + lowest * layer.repeat);
    lastRow = std::min(lastRow, layer.top + (highest + 1) * layer.repeat);
  }
  const auto covers = [&](int line) {
    return line >= sprite.top && line < sprite.top + sprite.height &&
           line >= 0 && line < layer.sourceRows;
  };
  for (int row = firstRow; row < lastRow; ++row) {
    const std::size_t at = static_cast<std::size_t>(row);
    const Update update = m_updates[at];
    if (update == Update::Full || !isShown(index, row)) {
      continue;
    }
    RowChange &change = m_frame->changes[at];
    const bool moved = update == Update::Spans && change.shift != 0;
    if (kept && !moved) {
      continue;
    }
    const int line = sourceRowOf(layer, row);
    const bool first = covers(line) && shown.first < shown.last;
    const bool second =
        covers(line + layer.sourceStep) && wrapped.first < wrapped.last;
    if (!first && !second) {
      continue;
    }
    Span span{first ? shown.first : wrapped.first,
              second ? wrapped.last : shown.last};
    if (old && moved) {
      span = {std::max(span.first + change.shift, 0),
              std::min(span.last + change.shift, m_frame->width)};
    }
    if (span.first >= span.last) {
      continue;
    }
    if (m_onlyLayers[at] != static_cast<int>(index)) {
      m_updates[at] = Update::Full;
      continue;
    }
    addSpan(change.spans, update == Update::Panned ? 1 : 2, span);
    if (update == Update::None || update == Update::Resave) {
      m_updates[at] = Update::Spans;
    }
  }
}

Span IndexedRasterizer::spriteColumns(const Layer &layer, const Placed &placed,
                                      const Sprite &sprite, bool wrapped) {
  const int first = wrapped ? placed.end : placed.start;
  const int last = wrapped ? placed.wrapEnd : placed.end;
  const int offset = placed.shift - (wrapped ? layer.sourceColumns : 0);
  const Span span{std::max(first, sprite.left - offset),
                  std::min(last, sprite.left + sprite.width - offset)};
  return span.first < span.last ? span : Span{};
}

void IndexedRasterizer::compareRows(const Display &display, std::size_t index,
                                    int firstRow, int lastRow) {
  if (m_sourceKept[index]) {
    return;
  }
  const Layer &layer = display.layers[index];
  if (layer.repeat > 1 || layer.sourceStep != 1 ||
      layer.stride != layer.sourceColumns) {
    for (int row = firstRow; row < lastRow; ++row) {
      compareRow(display, index, row, false);
    }
    return;
  }
  const std::size_t stride = static_cast<std::size_t>(layer.stride);
  const std::size_t offset =
      static_cast<std::size_t>(sourceRowOf(layer, firstRow)) * stride;
  const uint8_t *source = layer.pixels + offset;
  const uint8_t *saved = m_saved[index].data() + offset;
  const std::size_t end = static_cast<std::size_t>(lastRow - firstRow) * stride;
  std::size_t at = 0;
  while (at < end) {
    at += m_leading(source + at, saved + at, end - at);
    if (at >= end) {
      break;
    }
    const std::size_t line = at / stride;
    compareRow(display, index, firstRow + static_cast<int>(line), true);
    at = (line + 1) * stride;
  }
}

void IndexedRasterizer::compareRow(const Display &display, std::size_t index,
                                   int row, bool differs) {
  const Layer &layer = display.layers[index];
  const Placed &placed = m_placed[index];
  const std::size_t offset =
      static_cast<std::size_t>(sourceRowOf(layer, row)) *
          static_cast<std::size_t>(layer.stride) +
      static_cast<std::size_t>(placed.start + placed.shift);
  const uint8_t *in = layer.pixels + offset;
  const uint8_t *saved = m_saved[index].data() + offset;
  const int count = placed.end - placed.start;
  const std::size_t lead =
      m_leading(in, saved, static_cast<std::size_t>(count));
  if (lead == static_cast<std::size_t>(count)) {
    if (differs) {
      m_updates[static_cast<std::size_t>(row)] = Update::Resave;
    }
    return;
  }
  const bool shiftable = count > 2 * SHIFT_STEP && count % SHIFT_ALIGNMENT == 0;
  RowChange change;
  if (shiftable && m_streak != 0) {
    change = shifted(in, saved, count, m_streak);
    if (spanWidth(change) <= count / 2) {
      change.from = m_frame->changes[static_cast<std::size_t>(row)].from;
      m_frame->changes[static_cast<std::size_t>(row)] = change;
      m_updates[static_cast<std::size_t>(row)] = Update::Spans;
      return;
    }
  }
  const std::size_t trail = m_trailing(in + lead, saved + lead,
                                       static_cast<std::size_t>(count) - lead);
  change = RowChange{};
  change.spans[0] = {static_cast<int>(lead), count - static_cast<int>(trail)};
  int cost = change.spans[0].last - change.spans[0].first;
  if (cost > count / 2 && shiftable) {
    for (const int shift : {m_lastShift, -m_lastShift}) {
      const RowChange moved = shifted(in, saved, count, shift);
      const int movedCost = spanWidth(moved);
      if (movedCost < cost) {
        change = moved;
        cost = movedCost;
      }
      if (cost <= count / 2) {
        break;
      }
    }
    if (change.shift != 0) {
      m_lastShift = change.shift;
    }
  }
  m_streak = change.shift;
  change.from = m_frame->changes[static_cast<std::size_t>(row)].from;
  m_frame->changes[static_cast<std::size_t>(row)] = change;
  m_updates[static_cast<std::size_t>(row)] = Update::Spans;
}

RowChange IndexedRasterizer::shifted(const uint8_t *in, const uint8_t *saved,
                                     int count, int shift) const {
  const int step = std::abs(shift);
  const std::size_t kept = static_cast<std::size_t>(count - step);
  RowChange change;
  change.shift = shift;
  if (shift < 0) {
    const std::size_t lead = m_leading(in, saved + step, kept);
    change.spans[1] = {count - step, count};
    if (lead < kept) {
      const std::size_t trail =
          m_trailing(in + lead, saved + step + lead, kept - lead);
      change.spans[0] = {static_cast<int>(lead),
                         count - step - static_cast<int>(trail)};
    }
  } else {
    const std::size_t lead = m_leading(in + step, saved, kept);
    change.spans[0] = {0, step};
    if (lead < kept) {
      const std::size_t trail =
          m_trailing(in + step + lead, saved + lead, kept - lead);
      change.spans[1] = {step + static_cast<int>(lead),
                         count - static_cast<int>(trail)};
    }
  }
  return change;
}

bool IndexedRasterizer::hasChanged(const Display &display, int row) const {
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    if (!isShown(index, row)) {
      continue;
    }
    if (isRecolored(index, row)) {
      return true;
    }
    const Layer &layer = display.layers[index];
    if (!layer.pixels || m_sourceKept[index]) {
      continue;
    }
    const Placed &placed = m_placed[index];
    const int sourceRow = sourceRowOf(layer, row);
    const auto changed = [&](int from, int to, int line, int column) {
      if (from >= to || line < 0 || line >= layer.sourceRows) {
        return false;
      }
      const std::size_t offset = static_cast<std::size_t>(line) *
                                     static_cast<std::size_t>(layer.stride) +
                                 static_cast<std::size_t>(column);
      const std::size_t count = static_cast<std::size_t>(to - from);
      return m_leading(layer.pixels + offset, m_saved[index].data() + offset,
                       count) != count;
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
  if (m_topLayers[static_cast<std::size_t>(row)] == NO_LAYER) {
    std::memset(m_frame->pixels.data() +
                    static_cast<std::ptrdiff_t>(row) * m_frame->width,
                border, static_cast<std::size_t>(m_frame->width));
  }
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    if (isShown(index, row)) {
      drawWindow(display, index, row, m_placed[index].first,
                 m_placed[index].last);
    }
  }
}

void IndexedRasterizer::drawWindow(const Display &display, std::size_t index,
                                   int row, int from, int to) {
  const Layer &layer = display.layers[index];
  const Placed &placed = m_placed[index];
  Mapping &mapping = m_mappings[index];
  if (isRecolored(index, row)) {
    rowSlots(layer, index, row);
    drawSpan(layer, placed, m_rowMapping, row, from, to);
    overlaySprites(display, index, row, from, to, m_rowMapping.slots);
    uncolor(layer, index, row, mapped(layer, mapping));
    return;
  }
  drawChecked(layer, placed, mapping, row, from, to);
  if (!layer.sprites.empty()) {
    overlaySprites(display, index, row, from, to, mapped(layer, mapping).slots);
  }
}

const std::array<uint8_t, FRAME_COLORS> &
IndexedRasterizer::rowSlots(const Layer &layer, std::size_t index, int row) {
  const Mapping &base = mapped(layer, m_mappings[index]);
  if (!isRecolored(index, row)) {
    return base.slots;
  }
  if (m_rowLayer != static_cast<int>(index)) {
    m_rowMapping = base;
    m_rowMapping.block = false;
    m_rowLayer = static_cast<int>(index);
  }
  recolor(layer, index, row);
  return m_rowMapping.slots;
}

void IndexedRasterizer::overlaySprites(
    const Display &display, std::size_t index, int row, int from, int to,
    const std::array<uint8_t, FRAME_COLORS> &slots) {
  const Layer &layer = display.layers[index];
  if (layer.sprites.empty() || !layer.pixels) {
    return;
  }
  const Placed &placed = m_placed[index];
  uint8_t *out = m_frame->pixels.data() +
                 static_cast<std::ptrdiff_t>(row) * m_frame->width;
  const int sourceRow = sourceRowOf(layer, row);
  const auto overlay = [&](int first, int last, int line, int column) {
    const int low = std::max(first, from);
    const int high = std::min(last, to);
    if (low >= high || line < 0 || line >= layer.sourceRows) {
      return;
    }
    const int offset = column - first;
    for (const Sprite &sprite : layer.sprites) {
      const int y = line - sprite.top;
      if (y < 0 || y >= sprite.height) {
        continue;
      }
      const int a = std::max(low, sprite.left - offset);
      const int b = std::min(high, sprite.left + sprite.width - offset);
      if (a < b) {
        overlayRun(out + a,
                   sprite.pixels +
                       static_cast<std::ptrdiff_t>(y) * sprite.width + a +
                       offset - sprite.left,
                   static_cast<std::size_t>(b - a), slots);
      }
    }
  };
  overlay(placed.start, placed.end, sourceRow, placed.start + placed.shift);
  if (layer.wrap) {
    overlay(placed.end, placed.wrapEnd, sourceRow + layer.sourceStep,
            placed.end + placed.shift - layer.sourceColumns);
  }
}

void IndexedRasterizer::shiftRows(int firstRow, int lastRow, int shift) {
  if (shift == 0) {
    return;
  }
  const int moved = std::abs(shift);
  uint8_t *first = m_frame->pixels.data() +
                   static_cast<std::ptrdiff_t>(firstRow) * m_frame->width;
  const std::size_t kept =
      static_cast<std::size_t>((lastRow - firstRow) * m_frame->width - moved);
  if (shift < 0) {
    std::memmove(first, first + moved, kept);
  } else {
    std::memmove(first + moved, first, kept);
  }
}

void IndexedRasterizer::drawSpans(const Display &display, int row) {
  const std::size_t index =
      static_cast<std::size_t>(m_onlyLayers[static_cast<std::size_t>(row)]);
  const std::size_t count =
      m_updates[static_cast<std::size_t>(row)] == Update::Panned ? 1 : 2;
  if (!isPlain(display, index, row)) {
    const std::array<Span, 2> &spans =
        m_frame->changes[static_cast<std::size_t>(row)].spans;
    for (std::size_t at = 0; at < count; ++at) {
      if (spans[at].first < spans[at].last) {
        drawWindow(display, index, row, spans[at].first, spans[at].last);
      }
    }
    return;
  }
  const Layer &layer = display.layers[index];
  Mapping &mapping = m_mappings[index];
  uint8_t *out = m_frame->pixels.data() +
                 static_cast<std::ptrdiff_t>(row) * m_frame->width;
  const uint8_t *in =
      layer.pixels +
      static_cast<std::ptrdiff_t>(sourceRowOf(layer, row)) * layer.stride +
      m_placed[index].shift;
  const std::array<Span, 2> &spans =
      m_frame->changes[static_cast<std::size_t>(row)].spans;
  for (std::size_t at = 0; at < count; ++at) {
    const Span &span = spans[at];
    if (span.first >= span.last) {
      continue;
    }
    const std::size_t pixels = static_cast<std::size_t>(span.last - span.first);
    if (mapping.block) {
      const uint8_t seen =
          copyMasked(out + span.first, in + span.first, pixels, mapping.keep,
                     mapping.base, mapping.checked);
      if (!mapping.checked ||
          (seen & layer.mask & ~(blockSize(layer) - 1)) == 0) {
        continue;
      }
      mapping.block = false;
    }
    copyMapped(out + span.first, in + span.first, pixels,
               mapped(layer, mapping).slots);
  }
  if (m_anySprites) {
    overlaySpans(display, row);
  }
}

void IndexedRasterizer::overlaySpans(const Display &display, int row) {
  const std::size_t index =
      static_cast<std::size_t>(m_onlyLayers[static_cast<std::size_t>(row)]);
  if (display.layers[index].sprites.empty()) {
    return;
  }
  const std::size_t count =
      m_updates[static_cast<std::size_t>(row)] == Update::Panned ? 1 : 2;
  const std::array<Span, 2> &spans =
      m_frame->changes[static_cast<std::size_t>(row)].spans;
  const std::array<uint8_t, FRAME_COLORS> &slots =
      mapped(display.layers[index], m_mappings[index]).slots;
  for (std::size_t at = 0; at < count; ++at) {
    if (spans[at].first < spans[at].last) {
      overlaySprites(display, index, row, spans[at].first, spans[at].last,
                     slots);
    }
  }
}

void IndexedRasterizer::drawExposed(const Display &display) {
  if (m_exposedLayer == NO_LAYER) {
    return;
  }
  const std::size_t index = static_cast<std::size_t>(m_exposedLayer);
  m_exposedLayer = NO_LAYER;
  const Layer &layer = display.layers[index];
  const Placed &placed = m_placed[index];
  const Mapping &base = mapped(layer, m_mappings[index]);
  const std::vector<int> &starts = m_recolorStarts[index];
  const std::vector<int> &order = m_recolorOrder[index];
  const RowColor *changes = layer.rowColors.begin();
  uint8_t *saved = m_saved[index].data();
  const std::size_t stride = static_cast<std::size_t>(layer.stride);
  bool crossed = false;
  for (const Sprite &sprite : layer.sprites) {
    for (const bool wrapped : {false, true}) {
      const Span columns = spriteColumns(layer, placed, sprite, wrapped);
      crossed = crossed ||
                (columns.first < m_exposed.last &&
                 m_exposed.first < columns.last && (layer.wrap || !wrapped));
    }
  }
  for (int row = 0; row < m_frame->height; ++row) {
    if (m_updates[static_cast<std::size_t>(row)] != Update::Panned) {
      continue;
    }
    m_overrides.clear();
    if (!starts.empty()) {
      for (int at = starts[static_cast<std::size_t>(row)];
           at < starts[static_cast<std::size_t>(row) + 1]; ++at) {
        const RowColor &change = changes[order[static_cast<std::size_t>(at)]];
        if ((change.index & layer.mask) == change.index) {
          m_overrides.push_back(
              {change.index,
               m_recolorTargets[index][static_cast<std::size_t>(at)]});
        }
      }
    }
    const auto slotOf = [&](uint8_t value) {
      uint8_t chosen = base.slots[value];
      for (const std::array<uint8_t, 2> &override : m_overrides) {
        if ((value & layer.mask) == override[0]) {
          chosen = override[1];
        }
      }
      return chosen;
    };
    const uint8_t zero = slotOf(0);
    uint8_t *out = m_frame->pixels.data() +
                   static_cast<std::ptrdiff_t>(row) * m_frame->width;
    const int sourceRow = sourceRowOf(layer, row);
    for (int x = m_exposed.first; x < m_exposed.last; ++x) {
      int line = sourceRow;
      int column = x + placed.shift;
      if (x < placed.start || x >= placed.wrapEnd ||
          (x >= placed.end && !layer.wrap)) {
        if (layer.wrap) {
          out[x] = zero;
        }
        continue;
      }
      if (x >= placed.end) {
        line += layer.sourceStep;
        column -= layer.sourceColumns;
      }
      if (line < 0 || line >= layer.sourceRows) {
        if (layer.wrap) {
          out[x] = zero;
        }
        continue;
      }
      const std::size_t at = static_cast<std::size_t>(line) * stride +
                             static_cast<std::size_t>(column);
      saved[at] = layer.pixels[at];
      out[x] = slotOf(layer.pixels[at]);
    }
    if (crossed) {
      const std::array<uint8_t, FRAME_COLORS> &slots =
          rowSlots(layer, index, row);
      overlaySprites(display, index, row, m_exposed.first, m_exposed.last,
                     slots);
      if (isRecolored(index, row)) {
        uncolor(layer, index, row, base);
      }
    }
  }
}

uint8_t IndexedRasterizer::drawSpan(const Layer &layer, const Placed &placed,
                                    const Mapping &shown, int row, int from,
                                    int to) {
  const int low = std::max(from, placed.first);
  const int high = std::min(to, placed.last);
  if (low >= high) {
    return 0;
  }
  uint8_t *out = m_frame->pixels.data() +
                 static_cast<std::ptrdiff_t>(row) * m_frame->width;
  const uint8_t zero = shown.block ? shown.base : shown.slots[0];
  if (!layer.pixels) {
    std::memset(out + low, zero, static_cast<std::size_t>(high - low));
    return 0;
  }
  const auto fill = [&](int first, int last) {
    first = std::max(first, low);
    last = std::min(last, high);
    if (first < last) {
      std::memset(out + first, zero, static_cast<std::size_t>(last - first));
    }
  };
  const auto run = [&](int first, int last, int line, int column) -> uint8_t {
    const int clippedFirst = std::max(first, low);
    const int clippedLast = std::min(last, high);
    if (clippedFirst >= clippedLast) {
      return 0;
    }
    if (line < 0 || line >= layer.sourceRows) {
      if (layer.wrap) {
        fill(clippedFirst, clippedLast);
      }
      return 0;
    }
    const std::size_t count =
        static_cast<std::size_t>(clippedLast - clippedFirst);
    const uint8_t *in = layer.pixels +
                        static_cast<std::ptrdiff_t>(line) * layer.stride +
                        column + (clippedFirst - first);
    if (shown.block) {
      return copyMasked(out + clippedFirst, in, count, shown.keep, shown.base,
                        shown.checked);
    }
    copyMapped(out + clippedFirst, in, count, shown.slots);
    return 0;
  };
  const int sourceRow = sourceRowOf(layer, row);
  uint8_t seen =
      run(placed.start, placed.end, sourceRow, placed.start + placed.shift);
  if (layer.wrap) {
    fill(placed.first, placed.start);
    seen |= run(placed.end, placed.wrapEnd, sourceRow + layer.sourceStep,
                placed.end + placed.shift - layer.sourceColumns);
    fill(placed.wrapEnd, placed.last);
  }
  return seen;
}

void IndexedRasterizer::drawChecked(const Layer &layer, const Placed &placed,
                                    Mapping &mapping, int row, int from,
                                    int to) {
  const Mapping &shown = mapping.block ? mapping : mapped(layer, mapping);
  const uint8_t seen = drawSpan(layer, placed, shown, row, from, to);
  if (mapping.block && mapping.checked &&
      (seen & layer.mask & ~(blockSize(layer) - 1)) != 0) {
    mapping.block = false;
    drawSpan(layer, placed, mapped(layer, mapping), row, from, to);
  }
}

void IndexedRasterizer::recolor(const Layer &layer, std::size_t index,
                                int row) {
  const std::vector<int> &starts = m_recolorStarts[index];
  const std::vector<int> &order = m_recolorOrder[index];
  const RowColor *changes = layer.rowColors.begin();
  for (int at = starts[static_cast<std::size_t>(row)];
       at < starts[static_cast<std::size_t>(row) + 1]; ++at) {
    const RowColor &change = changes[order[static_cast<std::size_t>(at)]];
    const uint8_t recolored =
        m_recolorTargets[index][static_cast<std::size_t>(at)];
    forEachValue(layer.mask, change.index, [&](std::size_t value) {
      m_rowMapping.slots[value] = recolored;
    });
  }
}

void IndexedRasterizer::uncolor(const Layer &layer, std::size_t index, int row,
                                const Mapping &base) {
  const std::vector<int> &starts = m_recolorStarts[index];
  const std::vector<int> &order = m_recolorOrder[index];
  const RowColor *changes = layer.rowColors.begin();
  for (int at = starts[static_cast<std::size_t>(row)];
       at < starts[static_cast<std::size_t>(row) + 1]; ++at) {
    forEachValue(layer.mask, changes[order[static_cast<std::size_t>(at)]].index,
                 [&](std::size_t value) {
                   m_rowMapping.slots[value] = base.slots[value];
                 });
  }
}

void IndexedRasterizer::save(const Display &display, int row) {
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    const Layer &layer = display.layers[index];
    if (!layer.pixels || !isShown(index, row)) {
      continue;
    }
    const int sourceRow = sourceRowOf(layer, row);
    saveRow(layer, index, sourceRow);
    if (layer.wrap) {
      saveRow(layer, index, sourceRow + layer.sourceStep);
    }
  }
}

void IndexedRasterizer::saveWindow(const Display &display, std::size_t index,
                                   int row, int from, int to) {
  const Layer &layer = display.layers[index];
  if (!layer.pixels || from >= to) {
    return;
  }
  const Placed &placed = m_placed[index];
  const int sourceRow = sourceRowOf(layer, row);
  const auto copy = [&](int first, int last, int line, int column) {
    const int low = std::max(first, from);
    const int high = std::min(last, to);
    if (low >= high || line < 0 || line >= layer.sourceRows) {
      return;
    }
    const std::size_t offset = static_cast<std::size_t>(line) *
                                   static_cast<std::size_t>(layer.stride) +
                               static_cast<std::size_t>(column + low - first);
    std::memcpy(m_saved[index].data() + offset, layer.pixels + offset,
                static_cast<std::size_t>(high - low));
  };
  copy(placed.start, placed.end, sourceRow, placed.start + placed.shift);
  if (layer.wrap) {
    copy(placed.end, placed.wrapEnd, sourceRow + layer.sourceStep,
         placed.end + placed.shift - layer.sourceColumns);
  }
}

void IndexedRasterizer::saveRow(const Layer &layer, std::size_t index,
                                int sourceRow) {
  if (sourceRow < 0 || sourceRow >= layer.sourceRows) {
    return;
  }
  const std::size_t offset = static_cast<std::size_t>(sourceRow) *
                             static_cast<std::size_t>(layer.stride);
  std::memcpy(m_saved[index].data() + offset, layer.pixels + offset,
              static_cast<std::size_t>(layer.sourceColumns));
}

} // namespace openfranko::src::systems::graphics
