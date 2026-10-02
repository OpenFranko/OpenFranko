#include "FrameBuilder.h"

#include <algorithm>

namespace openfranko::src::systems::jaguar {
namespace {

constexpr int CLUT_SIZE = 256;
constexpr int FULL_MASK = 0xFF;
constexpr int PIXEL_BITS = 8;
constexpr uint8_t HALF_SCALE = SCALE_ONE / 2;
constexpr int NO_LAYER = -1;
constexpr int CHANNEL_STEP = 17;
constexpr int PIXELS_PER_PHRASE_16 = 4;
constexpr std::size_t AMIGA_COLORS = 4096;
constexpr uint16_t COLOR_MASK = 0xFFF;
constexpr int TOP_HALF_LINE = 0;
constexpr uint32_t PAIR_BYTES = 4;
constexpr std::array<uint32_t, 3> BANK_MASKS = {0x80, 0x40, 0xC0};
constexpr std::size_t MAX_TRANSLATED_BYTES = 128 * 1024;
constexpr uint32_t BYTE_COPIES = 0x01010101;
constexpr uintptr_t LONG_ALIGNMENT = 4;
constexpr int NO_SLOT = -1;
constexpr std::size_t HEADER_STOP = 3;
constexpr std::size_t FIRST_OBJECT = 4;

bool copperAllowed = true;

const std::array<uint16_t, AMIGA_COLORS> &rgb16Table() {
  static std::array<uint16_t, AMIGA_COLORS> table = [] {
    std::array<uint16_t, AMIGA_COLORS> colors{};
    for (std::size_t color = 0; color < AMIGA_COLORS; ++color) {
      colors[color] = toRgb16(static_cast<uint16_t>(color));
    }
    return colors;
  }();
  return table;
}

int ceilDiv(int value, int divisor) {
  return value >= 0 ? (value + divisor - 1) / divisor : -(-value / divisor);
}

int alignUp(int value, int multiple) {
  return ceilDiv(value, multiple) * multiple;
}

struct Rows {
  int first = 0;
  int last = 0;
};

struct Span {
  int first = 0;
  int last = 0;
  int owner = NO_LAYER;
};

struct PaletteSource {
  LayerArea area;
  uint8_t mask = FULL_MASK;
  std::vector<uint16_t> palette;
  std::vector<graphics::RowColor> rowColors;
};

struct PaletteCache {
  bool valid = false;
  int height = 0;
  int top = 0;
  int rowsPerLine = 1;
  int rows = 0;
  int firstHalfLine = 0;
  std::vector<PaletteSource> sources;
  std::array<uint16_t, CLUT_SIZE> clut{};
  std::vector<uint32_t> copper;
};

uint16_t effectiveColor(const graphics::Layer &layer, int value) {
  const std::size_t index = static_cast<std::size_t>(value & layer.mask);
  return index < layer.palette.size() ? layer.palette[index] & COLOR_MASK : 0;
}

bool sameRowColors(const std::vector<graphics::RowColor> &left,
                   const std::vector<graphics::RowColor> &right) {
  if (left.size() != right.size()) {
    return false;
  }
  for (std::size_t index = 0; index < left.size(); ++index) {
    const graphics::RowColor &a = left[index];
    const graphics::RowColor &b = right[index];
    if (a.row != b.row || a.index != b.index || a.color != b.color) {
      return false;
    }
  }
  return true;
}

bool sameArea(const LayerArea &left, const LayerArea &right) {
  return left.firstRow == right.firstRow && left.lastRow == right.lastRow &&
         left.firstColumn == right.firstColumn &&
         left.lastColumn == right.lastColumn;
}

Rows sourceRows(const graphics::Layer &layer) {
  Rows rows{layer.top, layer.top + layer.rows};
  if (!layer.pixels || layer.wrap) {
    return rows;
  }
  const int repeat = std::max(layer.repeat, 1);
  const int step = std::max(layer.sourceStep, 1);
  const int firstStep = layer.sourceY >= 0 ? 0 : ceilDiv(-layer.sourceY, step);
  const int lastStep =
      std::max(0, ceilDiv(layer.sourceRows - layer.sourceY, step));
  rows.first = std::max(rows.first, layer.top + firstStep * repeat);
  rows.last = std::min(rows.last, layer.top + lastStep * repeat);
  return rows;
}

Rows sourceColumns(const graphics::Layer &layer) {
  Rows columns{layer.left, layer.left + layer.columns};
  if (!layer.pixels || layer.wrap) {
    return columns;
  }
  columns.first = std::max(columns.first, layer.left - layer.sourceX);
  columns.last =
      std::min(columns.last, layer.left - layer.sourceX + layer.sourceColumns);
  return columns;
}

bool covers(const LayerArea &area, int row) {
  return row >= area.firstRow && row < area.lastRow &&
         area.firstColumn < area.lastColumn;
}

void ownerSpans(const std::vector<LayerArea> &areas, int firstRow, int lastRow,
                std::vector<int> &edges, std::vector<Span> &spans) {
  edges.clear();
  edges.push_back(firstRow);
  edges.push_back(lastRow);
  for (const LayerArea &area : areas) {
    for (const int edge : {area.firstRow, area.lastRow}) {
      if (edge > firstRow && edge < lastRow) {
        edges.push_back(edge);
      }
    }
  }
  std::sort(edges.begin(), edges.end());
  edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
  spans.clear();
  for (std::size_t edge = 0; edge + 1 < edges.size(); ++edge) {
    Span span{edges[edge], edges[edge + 1], NO_LAYER};
    for (int index = static_cast<int>(areas.size()) - 1; index >= 0; --index) {
      if (covers(areas[static_cast<std::size_t>(index)], span.first)) {
        span.owner = index;
        break;
      }
    }
    if (!spans.empty() && spans.back().owner == span.owner) {
      spans.back().last = span.last;
    } else {
      spans.push_back(span);
    }
  }
}

class Clut {
public:
  Clut(const graphics::Display &display, const Geometry &geometry,
       const Placement &placement, BuiltFrame &frame)
      : m_display(display), m_geometry(geometry), m_placement(placement),
        m_frame(frame), m_table(rgb16Table()) {}

  void load(int owner, int row) {
    const int loaded = base(owner, m_frame.clut);
    m_limit = std::max(loaded, override(owner, row, m_frame.clut));
  }

  void change(int owner, int row) {
    std::array<uint16_t, CLUT_SIZE> next{};
    const int loaded = base(owner, next);
    const int limit = std::max(loaded, override(owner, row, next));
    const int range = std::max(m_limit, limit);
    begin(row);
    for (int value = 0; value < range; ++value) {
      set(value, next[static_cast<std::size_t>(value)]);
    }
    end();
    m_limit = limit;
  }

  void recolor(int owner, int row, const graphics::RowColor *previous,
               const graphics::RowColor *previousEnd,
               const graphics::RowColor *current,
               const graphics::RowColor *currentEnd) {
    const graphics::Layer &layer = layerAt(owner);
    begin(row);
    for (const graphics::RowColor *change = previous; change != previousEnd;
         ++change) {
      forEachValue(layer, change->index, [&](int value) {
        if (!isChanged(layer, current, currentEnd, value)) {
          set(value, m_table[effectiveColor(layer, value)]);
        }
      });
    }
    for (const graphics::RowColor *change = current; change != currentEnd;
         ++change) {
      forEachValue(layer, change->index, [&](int value) {
        set(value, m_table[change->color & COLOR_MASK]);
      });
    }
    end();
  }

private:
  const graphics::Layer &layerAt(int owner) const {
    return m_display.layers[static_cast<std::size_t>(owner)];
  }

  static bool isChanged(const graphics::Layer &layer,
                        const graphics::RowColor *first,
                        const graphics::RowColor *last, int value) {
    for (const graphics::RowColor *change = first; change != last; ++change) {
      if ((change->index & layer.mask) == change->index &&
          (value & layer.mask) == change->index) {
        return true;
      }
    }
    return false;
  }

  template <typename Visit>
  static void forEachValue(const graphics::Layer &layer, uint8_t index,
                           Visit visit) {
    if ((index & layer.mask) != index) {
      return;
    }
    if (layer.mask == FULL_MASK) {
      visit(index);
      return;
    }
    for (int value = 0; value < CLUT_SIZE; ++value) {
      if ((value & layer.mask) == index) {
        visit(value);
      }
    }
  }

  int base(int owner, std::array<uint16_t, CLUT_SIZE> &colors) const {
    if (owner == NO_LAYER) {
      colors.fill(0);
      return 0;
    }
    const graphics::Layer &layer = layerAt(owner);
    if (layer.mask == FULL_MASK) {
      const std::size_t count =
          std::min<std::size_t>(layer.palette.size(), CLUT_SIZE);
      for (std::size_t value = 0; value < count; ++value) {
        colors[value] = m_table[layer.palette[value] & COLOR_MASK];
      }
      std::fill(colors.begin() + static_cast<std::ptrdiff_t>(count),
                colors.end(), 0);
      return static_cast<int>(count);
    }
    for (int value = 0; value < CLUT_SIZE; ++value) {
      colors[static_cast<std::size_t>(value)] =
          m_table[effectiveColor(layer, value)];
    }
    return CLUT_SIZE;
  }

  int override(int owner, int row,
               std::array<uint16_t, CLUT_SIZE> &colors) const {
    int limit = 0;
    if (owner == NO_LAYER) {
      return limit;
    }
    const graphics::Layer &layer = layerAt(owner);
    for (const graphics::RowColor &change : layer.rowColors) {
      if (change.row == row) {
        forEachValue(layer, change.index, [&](int value) {
          colors[static_cast<std::size_t>(value)] =
              m_table[change.color & COLOR_MASK];
          limit = std::max(limit, value + 1);
        });
      }
    }
    return limit;
  }

  void begin(int row) {
    m_header = m_frame.copper.size();
    const uint32_t line = static_cast<uint32_t>(
        m_geometry.firstHalfLine +
        2 * (m_placement.top + ceilDiv(row, m_placement.rowsPerLine)));
    m_frame.copper.push_back(line << 16);
    m_pairs.clear();
  }

  void set(int value, uint16_t color) {
    uint16_t &shown = m_frame.clut[static_cast<std::size_t>(value)];
    if (shown != color) {
      shown = color;
      const uint32_t pair = static_cast<uint32_t>(value) / 2;
      if (std::find(m_pairs.begin(), m_pairs.end(), pair) == m_pairs.end()) {
        m_pairs.push_back(pair);
      }
      if (color != 0 && value >= m_limit) {
        m_limit = value + 1;
      }
    }
  }

  void end() {
    if (m_pairs.empty()) {
      m_frame.copper.pop_back();
      return;
    }
    for (const uint32_t pair : m_pairs) {
      m_frame.copper.push_back(pair * PAIR_BYTES);
      m_frame.copper.push_back(static_cast<uint32_t>(m_frame.clut[2 * pair])
                                   << 16 |
                               m_frame.clut[2 * pair + 1]);
    }
    m_frame.copper[m_header] |= static_cast<uint32_t>(m_pairs.size());
  }

  const graphics::Display &m_display;
  const Geometry &m_geometry;
  const Placement &m_placement;
  BuiltFrame &m_frame;
  const std::array<uint16_t, AMIGA_COLORS> &m_table;
  std::size_t m_header = 0;
  int m_limit = 0;
  std::vector<uint32_t> m_pairs;
};

bool rowOrder(const graphics::RowColor &left, const graphics::RowColor &right) {
  return left.row < right.row;
}

void recolorSpan(Clut &clut, const Span &span,
                 const std::vector<graphics::RowColor> &changes,
                 std::vector<graphics::RowColor> &sorted) {
  const graphics::RowColor *begin = changes.data();
  const graphics::RowColor *end = changes.data() + changes.size();
  if (!std::is_sorted(begin, end, rowOrder)) {
    sorted.assign(begin, end);
    std::stable_sort(sorted.begin(), sorted.end(), rowOrder);
    begin = sorted.data();
    end = sorted.data() + sorted.size();
  }
  const graphics::RowColor *previous = begin;
  while (previous != end && previous->row < span.first) {
    ++previous;
  }
  const graphics::RowColor *previousEnd = previous;
  while (previousEnd != end && previousEnd->row == span.first) {
    ++previousEnd;
  }
  if (previous == previousEnd) {
    previous = previousEnd;
  }
  int lastChanged = span.first;
  const graphics::RowColor *current = previousEnd;
  while (current != end && current->row < span.last) {
    const graphics::RowColor *currentEnd = current;
    while (currentEnd != end && currentEnd->row == current->row) {
      ++currentEnd;
    }
    if (previous != previousEnd && current->row > lastChanged + 1) {
      clut.recolor(span.owner, lastChanged + 1, previous, previousEnd,
                   previousEnd, previousEnd);
      previous = previousEnd;
    }
    clut.recolor(span.owner, current->row, previous, previousEnd, current,
                 currentEnd);
    lastChanged = current->row;
    previous = current;
    previousEnd = currentEnd;
    current = currentEnd;
  }
  if (previous != previousEnd && lastChanged + 1 < span.last) {
    clut.recolor(span.owner, lastChanged + 1, previous, previousEnd,
                 previousEnd, previousEnd);
  }
}

void composePalettes(const graphics::Display &display, const Geometry &geometry,
                     const Placement &placement,
                     const std::vector<LayerArea> &areas, BuiltFrame &frame) {
  frame.copper.clear();
  const int perLine = placement.rowsPerLine;
  const int firstRow = std::max(0, -placement.top * perLine);
  const int lastRow =
      std::min(display.height, (geometry.rows - placement.top) * perLine);
  if (firstRow >= lastRow) {
    frame.clut.fill(0);
    frame.copper.push_back(COPPER_END);
    return;
  }
  static std::vector<int> edges;
  static std::vector<Span> spans;
  ownerSpans(areas, firstRow, lastRow, edges, spans);
  std::size_t first = 0;
  while (first < spans.size() && spans[first].owner == NO_LAYER) {
    ++first;
  }
  if (first == spans.size()) {
    frame.clut.fill(0);
    frame.copper.push_back(COPPER_END);
    return;
  }
  Clut clut(display, geometry, placement, frame);
  clut.load(spans[first].owner, spans[first].first);
  const std::array<uint16_t, CLUT_SIZE> start = frame.clut;
  static std::vector<graphics::RowColor> sorted;
  for (std::size_t index = first; index < spans.size(); ++index) {
    const Span &span = spans[index];
    if (span.owner == NO_LAYER) {
      continue;
    }
    if (index > first) {
      clut.change(span.owner, span.first);
    }
    const std::vector<graphics::RowColor> &changes =
        display.layers[static_cast<std::size_t>(span.owner)].rowColors;
    if (!changes.empty()) {
      recolorSpan(clut, span, changes, sorted);
    }
  }
  frame.copper.push_back(COPPER_END);
  frame.clut = start;
}

bool isCached(const PaletteCache &cache, const graphics::Display &display,
              const Geometry &geometry, const Placement &placement,
              const std::vector<LayerArea> &areas) {
  if (!cache.valid || cache.height != display.height ||
      cache.top != placement.top ||
      cache.rowsPerLine != placement.rowsPerLine ||
      cache.rows != geometry.rows ||
      cache.firstHalfLine != geometry.firstHalfLine ||
      cache.sources.size() != display.layers.size()) {
    return false;
  }
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    const PaletteSource &source = cache.sources[index];
    const graphics::Layer &layer = display.layers[index];
    if (!sameArea(source.area, areas[index]) || source.mask != layer.mask ||
        source.palette != layer.palette ||
        !sameRowColors(source.rowColors, layer.rowColors)) {
      return false;
    }
  }
  return true;
}

void buildPalettes(const graphics::Display &display, const Geometry &geometry,
                   const Placement &placement,
                   const std::vector<LayerArea> &areas, BuiltFrame &frame) {
  static PaletteCache cache;
  if (isCached(cache, display, geometry, placement, areas)) {
    frame.clut = cache.clut;
    frame.copper = cache.copper;
    return;
  }
  composePalettes(display, geometry, placement, areas, frame);
  cache.valid = true;
  cache.height = display.height;
  cache.top = placement.top;
  cache.rowsPerLine = placement.rowsPerLine;
  cache.rows = geometry.rows;
  cache.firstHalfLine = geometry.firstHalfLine;
  cache.sources.resize(display.layers.size());
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    PaletteSource &source = cache.sources[index];
    const graphics::Layer &layer = display.layers[index];
    source.area = areas[index];
    source.mask = layer.mask;
    source.palette = layer.palette;
    source.rowColors = layer.rowColors;
  }
  cache.clut = frame.clut;
  cache.copper = frame.copper;
}

void addObject(const graphics::Layer &layer, const LayerArea &area,
               const Placement &placement, const Geometry &geometry,
               uint32_t solidPhrase, bool transparent, ObjectList &list) {
  if (area.firstRow >= area.lastRow || area.firstColumn >= area.lastColumn) {
    return;
  }
  const int repeat = std::max(layer.repeat, 1);
  const int step = std::max(layer.sourceStep, 1);
  const int perLine = placement.rowsPerLine;
  const int lineRepeat = repeat % perLine == 0 ? repeat / perLine : 1;
  const int lineStep = repeat % perLine == 0 ? step : step * perLine;
  const int lines = (area.lastRow - area.firstRow) / perLine;
  const int width = area.lastColumn - area.firstColumn;
  BitmapObject object;
  object.y =
      geometry.firstHalfLine + 2 * (placement.top + area.firstRow / perLine);
  object.x = placement.left + area.firstColumn / placement.halfWidth;
  object.depth = Depth::Bits8;
  if (!layer.pixels) {
    object.data = solidPhrase;
    object.pitch = 0;
    object.dataWidth = 0;
    object.imageWidth =
        (width / placement.halfWidth + PHRASE_BYTES - 1) / PHRASE_BYTES;
    object.height = lines;
    list.addBitmap(object);
    return;
  }
  const int firstStep = (area.firstRow - layer.top) / repeat;
  const int sourceRow = layer.sourceY + firstStep * step;
  const int sourceColumn = layer.sourceX + area.firstColumn - layer.left;
  const uint32_t address =
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(layer.pixels)) +
      static_cast<uint32_t>(sourceRow * layer.stride + sourceColumn);
  int offset = static_cast<int>(address % PHRASE_BYTES);
  object.transparent = transparent;
  object.scaled = lineRepeat > 1 || placement.halfWidth > 1;
  object.imageWidth = (offset + width + PHRASE_BYTES - 1) / PHRASE_BYTES;
  if (offset % 2 != 0 && !object.scaled) {
    --offset;
    --object.x;
  }
  object.data = address - static_cast<uint32_t>(address % PHRASE_BYTES);
  object.firstPixel = offset * PIXEL_BITS;
  object.dataWidth = layer.stride * lineStep / PHRASE_BYTES;
  object.height = ceilDiv(lines, lineRepeat);
  if (object.scaled) {
    object.horizontalScale = placement.halfWidth > 1 ? HALF_SCALE : SCALE_ONE;
    object.verticalScale = static_cast<uint8_t>(SCALE_ONE * lineRepeat);
  }
  list.addBitmap(object);
}

void addLineColors(const LayerArea &area, const Placement &placement,
                   const Geometry &geometry, const uint8_t *phrases,
                   ObjectList &list) {
  const int perLine = placement.rowsPerLine;
  const int width =
      ceilDiv(area.lastColumn - area.firstColumn, placement.halfWidth);
  BitmapObject object;
  object.y =
      geometry.firstHalfLine + 2 * (placement.top + area.firstRow / perLine);
  object.x = placement.left + area.firstColumn / placement.halfWidth;
  object.depth = Depth::Bits16;
  object.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(phrases));
  object.pitch = 0;
  object.dataWidth = 1;
  object.imageWidth = ceilDiv(width, PIXELS_PER_PHRASE_16);
  object.height = (area.lastRow - area.firstRow) / perLine;
  list.addBitmap(object);
}

void writeLineColors(const graphics::Layer &layer, const LayerArea &area,
                     const Placement &placement, uint8_t *target,
                     BuiltFrame &frame) {
  const int perLine = placement.rowsPerLine;
  const int lines = (area.lastRow - area.firstRow) / perLine;
  const std::array<uint16_t, AMIGA_COLORS> &table = rgb16Table();
  static std::vector<uint16_t> colors;
  colors.assign(static_cast<std::size_t>(lines),
                table[effectiveColor(layer, 0)]);
  for (const graphics::RowColor &change : layer.rowColors) {
    const int offset = change.row - area.firstRow;
    if (offset >= 0 && offset < lines * perLine && offset % perLine == 0) {
      colors[static_cast<std::size_t>(offset / perLine)] =
          table[change.color & COLOR_MASK];
    }
  }
  if (frame.lineTarget == target && frame.lineColors == colors) {
    return;
  }
  for (std::size_t line = 0; line < colors.size(); ++line) {
    const uint8_t high = static_cast<uint8_t>(colors[line] >> 8);
    const uint8_t low = static_cast<uint8_t>(colors[line]);
    uint8_t *phrase = target + line * PHRASE_BYTES;
    for (int pixel = 0; pixel < PIXELS_PER_PHRASE_16; ++pixel) {
      phrase[2 * pixel] = high;
      phrase[2 * pixel + 1] = low;
    }
  }
  frame.lineColors = colors;
  frame.lineTarget = target;
}

uint8_t *linePhrases(const FrameMemory &memory, int lines) {
  if (!memory.linePhrases || lines <= 0) {
    return nullptr;
  }
  const uintptr_t raw = reinterpret_cast<uintptr_t>(memory.linePhrases);
  const uintptr_t skip = (PHRASE_BYTES - raw % PHRASE_BYTES) % PHRASE_BYTES;
  const int capacity = memory.lineCapacity - (skip != 0 ? 1 : 0);
  return lines <= capacity ? memory.linePhrases + skip : nullptr;
}

struct LayerPlan {
  bool merged = false;
  uint32_t bank = 0;
  int solidSlot = NO_SLOT;
  uint8_t *buffer = nullptr;
};

bool isKept(const graphics::Layer &layer, std::size_t value) {
  return (value & layer.mask) == value;
}

class Banking {
public:
  bool fits(const graphics::Layer &layer, uint32_t bank) const {
    for (std::size_t value = 0; value < layer.palette.size(); ++value) {
      if (!isKept(layer, value)) {
        continue;
      }
      const std::size_t slot = value ^ bank;
      if (slot >= CLUT_SIZE) {
        return false;
      }
      if (m_used[slot] &&
          m_colors[slot] != (layer.palette[value] & COLOR_MASK)) {
        return false;
      }
    }
    return true;
  }

  void claim(const graphics::Layer &layer, uint32_t bank) {
    for (std::size_t value = 0; value < layer.palette.size(); ++value) {
      if (!isKept(layer, value)) {
        continue;
      }
      const std::size_t slot = value ^ bank;
      m_used[slot] = true;
      m_colors[slot] = layer.palette[value] & COLOR_MASK;
    }
  }

  int solidSlot(uint16_t color) {
    for (int slot = CLUT_SIZE - 1; slot >= 0; --slot) {
      const std::size_t index = static_cast<std::size_t>(slot);
      if (m_used[index] && m_colors[index] == color) {
        return slot;
      }
    }
    for (int slot = CLUT_SIZE - 1; slot >= 0; --slot) {
      const std::size_t index = static_cast<std::size_t>(slot);
      if (!m_used[index]) {
        m_used[index] = true;
        m_colors[index] = color;
        return slot;
      }
    }
    return NO_SLOT;
  }

  void palette(const graphics::Layer *base,
               std::vector<uint16_t> &colors) const {
    colors.assign(m_colors.begin(), m_colors.end());
    if (!base) {
      return;
    }
    for (int slot = 0; slot < CLUT_SIZE; ++slot) {
      const std::size_t index = static_cast<std::size_t>(slot);
      if (!m_used[index]) {
        colors[index] = effectiveColor(*base, slot);
      }
    }
  }

private:
  std::array<uint16_t, CLUT_SIZE> m_colors{};
  std::array<bool, CLUT_SIZE> m_used{};
};

bool isEmpty(const LayerArea &area) {
  return area.firstRow >= area.lastRow || area.firstColumn >= area.lastColumn;
}

std::size_t sourceBytes(const graphics::Layer &layer) {
  return static_cast<std::size_t>(std::max(layer.stride, 0)) *
         static_cast<std::size_t>(std::max(layer.sourceRows, 0));
}

bool isTranslatable(const graphics::Layer &layer) {
  const std::size_t bytes = sourceBytes(layer);
  return layer.rowColors.empty() && bytes != 0 &&
         bytes <= MAX_TRANSLATED_BYTES &&
         reinterpret_cast<uintptr_t>(layer.pixels) % LONG_ALIGNMENT == 0;
}

std::size_t baseLayer(const graphics::Display &display,
                      const std::vector<LayerArea> &areas) {
  std::size_t base = display.layers.size();
  int baseRows = 0;
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    const LayerArea &area = areas[index];
    if (!display.layers[index].pixels || isEmpty(area)) {
      continue;
    }
    const int rows = area.lastRow - area.firstRow;
    if (rows > baseRows) {
      base = index;
      baseRows = rows;
    }
  }
  return base;
}

bool takesLineColors(const graphics::Layer &layer, const LayerPlan &plan) {
  if (layer.rowColors.empty() || !layer.pixels || layer.mask != FULL_MASK ||
      !plan.merged || plan.bank != 0 || plan.buffer) {
    return false;
  }
  for (const graphics::RowColor &change : layer.rowColors) {
    if (change.index != 0) {
      return false;
    }
  }
  return true;
}

std::size_t planBanks(const graphics::Display &display,
                      const std::vector<LayerArea> &areas,
                      const FrameMemory &memory, std::vector<LayerPlan> &plans,
                      Banking &banking) {
  plans.assign(display.layers.size(), LayerPlan{});
  const std::size_t base = baseLayer(display, areas);
  if (base < display.layers.size()) {
    banking.claim(display.layers[base], 0);
    plans[base].merged = true;
  }
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    const graphics::Layer &layer = display.layers[index];
    if (index == base || isEmpty(areas[index])) {
      continue;
    }
    LayerPlan &plan = plans[index];
    if (!layer.pixels) {
      const uint16_t color = static_cast<uint16_t>(
          layer.palette.empty() ? 0 : layer.palette[0] & COLOR_MASK);
      plan.solidSlot = banking.solidSlot(color);
      plan.merged = plan.solidSlot != NO_SLOT;
      continue;
    }
    if (banking.fits(layer, 0)) {
      banking.claim(layer, 0);
      plan.merged = true;
      continue;
    }
    if (!memory.buffers || !isTranslatable(layer)) {
      continue;
    }
    for (const uint32_t bank : BANK_MASKS) {
      if (!banking.fits(layer, bank)) {
        continue;
      }
      plan.buffer = memory.buffers->buffer(layer.pixels, sourceBytes(layer));
      if (plan.buffer) {
        banking.claim(layer, bank);
        plan.merged = true;
        plan.bank = bank;
      }
      break;
    }
  }
  return base;
}

uint32_t borderColor(uint16_t color) {
  const uint32_t red = ((color >> 8) & 0xF) * CHANNEL_STEP;
  const uint32_t green = ((color >> 4) & 0xF) * CHANNEL_STEP;
  const uint32_t blue = (color & 0xF) * CHANNEL_STEP;
  return blue << 16 | green << 8 | red;
}

} // namespace

Placement placeDisplay(const graphics::Display &display,
                       const Geometry &geometry) {
  Placement placement;
  placement.halfWidth = display.width > HIRES_THRESHOLD ? 2 : 1;
  placement.rowsPerLine = placement.halfWidth == 1 &&
                                  display.displayHeight > 0 &&
                                  display.height >= 2 * display.displayHeight
                              ? 2
                              : 1;
  placement.left = (geometry.columns - display.width / placement.halfWidth) / 2;
  placement.top = (geometry.rows - display.height / placement.rowsPerLine) / 2;
  return placement;
}

LayerArea visibleArea(const graphics::Display &display,
                      const graphics::Layer &layer, const Placement &placement,
                      const Geometry &geometry) {
  const Rows rows = sourceRows(layer);
  const Rows columns = sourceColumns(layer);
  const int perLine = placement.rowsPerLine;
  LayerArea area;
  area.firstRow =
      alignUp(std::max({0, rows.first, -placement.top * perLine}), perLine);
  area.lastRow = alignUp(std::min({display.height, rows.last,
                                   (geometry.rows - placement.top) * perLine}),
                         perLine);
  area.firstColumn =
      std::max({0, columns.first, -placement.left * placement.halfWidth});
  area.lastColumn =
      std::min({display.width, columns.last,
                (geometry.columns - placement.left) * placement.halfWidth});
  if (area.lastRow < area.firstRow) {
    area.lastRow = area.firstRow;
  }
  if (area.lastColumn < area.firstColumn) {
    area.lastColumn = area.firstColumn;
  }
  return area;
}

bool sameLayout(const graphics::Display &left, const graphics::Display &right) {
  if (left.width != right.width || left.height != right.height ||
      left.displayHeight != right.displayHeight ||
      left.border != right.border ||
      left.layers.size() != right.layers.size()) {
    return false;
  }
  for (std::size_t index = 0; index < left.layers.size(); ++index) {
    const graphics::Layer &a = left.layers[index];
    const graphics::Layer &b = right.layers[index];
    if (a.pixels != b.pixels || a.stride != b.stride ||
        a.sourceColumns != b.sourceColumns || a.sourceRows != b.sourceRows ||
        a.sourceX != b.sourceX || a.sourceY != b.sourceY ||
        a.sourceStep != b.sourceStep || a.repeat != b.repeat ||
        a.wrap != b.wrap || a.left != b.left || a.top != b.top ||
        a.columns != b.columns || a.rows != b.rows || a.mask != b.mask ||
        a.palette != b.palette || !sameRowColors(a.rowColors, b.rowColors)) {
      return false;
    }
  }
  return true;
}

void allowCopper(bool allowed) { copperAllowed = allowed; }

void buildFrame(const graphics::Display &display, const Geometry &geometry,
                const FrameMemory &memory, const Overlay *overlays,
                std::size_t overlayCount, BuiltFrame &frame) {
  const Placement placement = placeDisplay(display, geometry);
  static std::vector<LayerArea> areas;
  areas.clear();
  for (const graphics::Layer &layer : display.layers) {
    areas.push_back(visibleArea(display, layer, placement, geometry));
  }

  static std::vector<LayerPlan> plans;
  static std::vector<uint16_t> merged;
  static graphics::Display adjusted;
  Banking banking;
  const std::size_t base = planBanks(display, areas, memory, plans, banking);
  banking.palette(
      base < display.layers.size() ? &display.layers[base] : nullptr, merged);
  adjusted = display;
  frame.translations.clear();
  const bool combined =
      std::count_if(plans.begin(), plans.end(),
                    [](const LayerPlan &plan) { return plan.merged; }) > 1;
  std::size_t lineLayer = display.layers.size();
  uint8_t *lines = nullptr;
  for (std::size_t index = 0; index < plans.size(); ++index) {
    const LayerPlan &plan = plans[index];
    const graphics::Layer &source = display.layers[index];
    graphics::Layer &layer = adjusted.layers[index];
    if (plan.merged) {
      layer.palette = merged;
      if (combined) {
        layer.mask = FULL_MASK;
      }
    }
    if (plan.buffer) {
      frame.translations.push_back(
          {source.pixels, plan.buffer, sourceBytes(source),
           source.mask * BYTE_COPIES, plan.bank * BYTE_COPIES});
      layer.pixels = plan.buffer;
    }
    if (!lines && takesLineColors(source, plan)) {
      const LayerArea &area = areas[index];
      lines = linePhrases(memory, (area.lastRow - area.firstRow) /
                                      placement.rowsPerLine);
      if (lines) {
        lineLayer = index;
        layer.rowColors.clear();
        writeLineColors(source, area, placement, lines, frame);
      }
    }
  }

  frame.background = rgb16Table()[display.border & COLOR_MASK];
  frame.border = borderColor(display.border);
  buildPalettes(adjusted, geometry, placement, areas, frame);
  if (!copperAllowed) {
    frame.copper.assign(1, COPPER_END);
  }
  const bool copper = frame.copper.size() > 1;

  static ObjectList list(memory.liveAddress);
  list.reset(memory.liveAddress);
  list.addBranch(geometry.lastHalfLine, Branch::Below, HEADER_STOP);
  list.addBranch(geometry.firstHalfLine, Branch::Above, HEADER_STOP);
  list.addBranch(TOP_HALF_LINE, Branch::Below, FIRST_OBJECT);
  list.addStop();
  if (copper) {
    list.addGpuObject(ALL_LINES, 0);
  }
  for (std::size_t index = 0; index < adjusted.layers.size(); ++index) {
    if (index == lineLayer) {
      addLineColors(areas[index], placement, geometry, lines, list);
    }
    const int slot =
        plans[index].solidSlot == NO_SLOT ? 0 : plans[index].solidSlot;
    addObject(adjusted.layers[index], areas[index], placement, geometry,
              memory.solidPhrases + static_cast<uint32_t>(slot) * PHRASE_BYTES,
              index == lineLayer, list);
  }
  for (std::size_t index = 0; index < overlayCount; ++index) {
    const Overlay &overlay = overlays[index];
    BitmapObject object;
    object.data = overlay.pixels;
    object.depth = Depth::Bits16;
    object.x = overlay.column;
    object.y = geometry.firstHalfLine + 2 * overlay.row;
    object.height = overlay.height;
    object.dataWidth = overlay.width / PIXELS_PER_PHRASE_16;
    object.imageWidth = overlay.width / PIXELS_PER_PHRASE_16;
    list.addBitmap(object);
  }
  list.addStop();
  frame.phrases.assign(list.phrases().begin(), list.phrases().end());
}

void translateOnCpu(const Translation &translation) {
  const uint8_t keep = static_cast<uint8_t>(translation.keep);
  const uint8_t flip = static_cast<uint8_t>(translation.flip);
  for (std::size_t at = 0; at < translation.bytes; ++at) {
    translation.target[at] =
        static_cast<uint8_t>((translation.source[at] & keep) ^ flip);
  }
}

} // namespace openfranko::src::systems::jaguar
