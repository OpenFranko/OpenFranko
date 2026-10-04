#include "FrameBuilder.h"

#include "../Multiply.h"

#include <algorithm>
#include <cstring>

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
constexpr uintptr_t LONG_ALIGNMENT = 4;
constexpr int NO_SLOT = -1;
constexpr int NO_OBJECT = -1;
constexpr int SHORT_LIMIT = 0x7FFF;
constexpr int MAX_SCALE = 255;
constexpr std::size_t HEADER_STOP = 3;
constexpr std::size_t FIRST_OBJECT = 4;
constexpr int MASK_PIXELS = 8;

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

uint32_t everyByte(uint32_t value) {
  value &= 0xFF;
  value |= value << 8;
  return value | value << 16;
}

int scaled(int value, int factor) {
  if (factor == 1) {
    return value;
  }
  if (factor == 2) {
    return value * 2;
  }
  return value * factor;
}

int divided(int value, int divisor) {
  if (divisor == 1) {
    return value;
  }
  if (divisor == 2) {
    return value / 2;
  }
  return value / divisor;
}

bool divides(int value, int divisor) {
  if (divisor == 1) {
    return true;
  }
  if (divisor == 2) {
    return (value & 1) == 0;
  }
  return value % divisor == 0;
}

int product(int left, int right) {
  if (left >= -SHORT_LIMIT && left <= SHORT_LIMIT && right >= -SHORT_LIMIT &&
      right <= SHORT_LIMIT) {
    return systems::multiplySigned16(static_cast<int16_t>(left),
                                     static_cast<int16_t>(right));
  }
  return left * right;
}

int ceilDiv(int value, int divisor) {
  if (divisor == 1) {
    return value;
  }
  if (divisor == 2) {
    return value >= 0 ? (value + 1) / 2 : -(-value / 2);
  }
  return value >= 0 ? (value + divisor - 1) / divisor : -(-value / divisor);
}

int alignUp(int value, int multiple) {
  return scaled(ceilDiv(value, multiple), multiple);
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
  graphics::RowColors rowColors;
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

bool sameRowColors(const graphics::RowColors &left,
                   const graphics::RowColors &right) {
  if (left.shares(right)) {
    return true;
  }
  if (left.size() != right.size()) {
    return false;
  }
  const graphics::RowColor *b = right.begin();
  for (const graphics::RowColor &a : left) {
    if (a.row != b->row || a.index != b->index || a.color != b->color) {
      return false;
    }
    ++b;
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
  rows.first = std::max(rows.first, layer.top + product(firstStep, repeat));
  rows.last = std::min(rows.last, layer.top + product(lastStep, repeat));
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
                 const graphics::RowColors &changes,
                 std::vector<graphics::RowColor> &sorted) {
  const graphics::RowColor *begin = changes.begin();
  const graphics::RowColor *end = changes.end();
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
    const graphics::RowColors &changes =
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

bool makeObject(const graphics::Layer &layer, const uint8_t *pixels,
                const LayerArea &area, const Placement &placement,
                const Geometry &geometry, uint32_t solidPhrase,
                bool transparent, BitmapObject &object) {
  if (area.firstRow >= area.lastRow || area.firstColumn >= area.lastColumn) {
    return false;
  }
  const int repeat = std::max(layer.repeat, 1);
  const int step = std::max(layer.sourceStep, 1);
  const int perLine = placement.rowsPerLine;
  const bool whole = divides(repeat, perLine);
  const int lineRepeat = whole ? divided(repeat, perLine) : 1;
  const int lineStep = whole ? step : scaled(step, perLine);
  const int lines = divided(area.lastRow - area.firstRow, perLine);
  const int width = area.lastColumn - area.firstColumn;
  object = BitmapObject{};
  object.y = geometry.firstHalfLine +
             2 * (placement.top + divided(area.firstRow, perLine));
  object.x = placement.left + divided(area.firstColumn, placement.halfWidth);
  object.depth = Depth::Bits8;
  if (!pixels) {
    object.data = solidPhrase;
    object.pitch = 0;
    object.dataWidth = 0;
    object.imageWidth =
        (divided(width, placement.halfWidth) + PHRASE_BYTES - 1) / PHRASE_BYTES;
    object.height = lines;
    return true;
  }
  const int firstStep = divided(area.firstRow - layer.top, repeat);
  const int sourceRow = layer.sourceY + product(firstStep, step);
  const int sourceColumn = layer.sourceX + area.firstColumn - layer.left;
  const uint32_t address =
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pixels)) +
      static_cast<uint32_t>(product(sourceRow, layer.stride) + sourceColumn);
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
  object.dataWidth = product(layer.stride, lineStep) / PHRASE_BYTES;
  object.height = ceilDiv(lines, lineRepeat);
  if (object.scaled) {
    object.horizontalScale = placement.halfWidth > 1 ? HALF_SCALE : SCALE_ONE;
    object.verticalScale = static_cast<uint8_t>(SCALE_ONE * lineRepeat);
    --object.height;
  }
  return true;
}

BitmapObject parkedObject(const Geometry &geometry, uint32_t solidPhrase,
                          const Placement &placement) {
  BitmapObject object;
  object.y = geometry.lastHalfLine + 2;
  object.data = solidPhrase;
  object.imageWidth = 1;
  object.height = 1;
  object.transparent = true;
  object.scaled = placement.halfWidth > 1;
  if (object.scaled) {
    object.horizontalScale = HALF_SCALE;
  }
  return object;
}

BitmapObject generalSpriteObject(const graphics::Layer &layer,
                                 const graphics::Sprite &sprite,
                                 const LayerArea &owner,
                                 const Placement &placement,
                                 const Geometry &geometry,
                                 uint32_t solidPhrase) {
  static graphics::Layer view;
  view.pixels = sprite.pixels;
  view.stride = sprite.width;
  view.sourceColumns = sprite.width;
  view.sourceRows = sprite.height;
  view.sourceX = layer.sourceX - sprite.left;
  view.sourceY = layer.sourceY - sprite.top;
  view.sourceStep = layer.sourceStep;
  view.repeat = layer.repeat;
  view.left = layer.left;
  view.top = layer.top;
  view.columns = layer.columns;
  view.rows = layer.rows;
  const Rows rows = sourceRows(view);
  const Rows columns = sourceColumns(view);
  const int perLine = placement.rowsPerLine;
  LayerArea area;
  area.firstRow = std::max(alignUp(rows.first, perLine), owner.firstRow);
  area.lastRow = std::min(alignUp(rows.last, perLine), owner.lastRow);
  area.firstColumn = std::max(columns.first, owner.firstColumn);
  area.lastColumn = std::min(columns.last, owner.lastColumn);
  BitmapObject object;
  if (!makeObject(view, view.pixels, area, placement, geometry, solidPhrase,
                  true, object)) {
    return parkedObject(geometry, solidPhrase, placement);
  }
  return object;
}

struct SpriteFields {
  uint32_t data = 0;
  int x = 0;
  int y = 0;
  int height = 0;
  int dataWidth = 0;
  int imageWidth = 0;
  int firstPixel = 0;
};

bool isPlainSprite(const graphics::Sprite &sprite) {
  return sprite.pixels && sprite.width % PHRASE_BYTES == 0;
}

bool hasFastSprites(const graphics::Layer &layer, const Placement &placement) {
  return layer.repeat == 1 && layer.sourceStep == 1 && !layer.wrap &&
         placement.halfWidth == 1;
}

bool fastSprite(const graphics::Layer &layer, const graphics::Sprite &sprite,
                const LayerArea &owner, const Placement &placement,
                const Geometry &geometry, SpriteFields &fields) {
  const int left = layer.left + sprite.left - layer.sourceX;
  const int top = layer.top + sprite.top - layer.sourceY;
  const int firstColumn = std::max(left, owner.firstColumn);
  const int lastColumn = std::min(left + sprite.width, owner.lastColumn);
  int firstRow = std::max(top, owner.firstRow);
  int lastRow = std::min(top + sprite.height, owner.lastRow);
  const int lineShift = placement.rowsPerLine == 2 ? 1 : 0;
  firstRow += firstRow & lineShift;
  lastRow += lastRow & lineShift;
  if (firstColumn >= lastColumn || firstRow >= lastRow) {
    return false;
  }
  const uint32_t address =
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(sprite.pixels)) +
      static_cast<uint32_t>(
          systems::multiplySigned16(static_cast<int16_t>(firstRow - top),
                                    sprite.width) +
          (firstColumn - left));
  int offset = static_cast<int>(address % PHRASE_BYTES);
  fields.data = address - static_cast<uint32_t>(offset);
  fields.imageWidth =
      (offset + lastColumn - firstColumn + PHRASE_BYTES - 1) / PHRASE_BYTES;
  fields.x = placement.left + firstColumn;
  if (offset % 2 != 0) {
    --offset;
    --fields.x;
  }
  fields.firstPixel = offset * PIXEL_BITS;
  fields.y =
      geometry.firstHalfLine + 2 * (placement.top + (firstRow >> lineShift));
  fields.height = (lastRow - firstRow) >> lineShift;
  fields.dataWidth = (sprite.width << lineShift) / PHRASE_BYTES;
  return true;
}

BitmapObject spriteObject(const graphics::Layer &layer,
                          const graphics::Sprite &sprite,
                          const LayerArea &owner, const Placement &placement,
                          const Geometry &geometry, uint32_t solidPhrase) {
  if (!isPlainSprite(sprite)) {
    return parkedObject(geometry, solidPhrase, placement);
  }
  if (!hasFastSprites(layer, placement)) {
    return generalSpriteObject(layer, sprite, owner, placement, geometry,
                               solidPhrase);
  }
  SpriteFields fields;
  if (!fastSprite(layer, sprite, owner, placement, geometry, fields)) {
    return parkedObject(geometry, solidPhrase, placement);
  }
  BitmapObject object;
  object.data = fields.data;
  object.x = fields.x;
  object.y = fields.y;
  object.height = fields.height;
  object.dataWidth = fields.dataWidth;
  object.imageWidth = fields.imageWidth;
  object.firstPixel = fields.firstPixel;
  object.transparent = true;
  return object;
}

int addObject(const graphics::Layer &layer, const uint8_t *pixels,
              const LayerArea &area, const Placement &placement,
              const Geometry &geometry, uint32_t solidPhrase, bool transparent,
              ObjectList &list) {
  BitmapObject object;
  if (!makeObject(layer, pixels, area, placement, geometry, solidPhrase,
                  transparent, object)) {
    return NO_OBJECT;
  }
  return static_cast<int>(list.addBitmap(object));
}

int lineShift(const Placement &placement) {
  return placement.rowsPerLine == 2 ? 1 : 0;
}

BitmapObject lineObject(const LayerArea &area, const Placement &placement,
                        const Geometry &geometry, const uint8_t *phrases) {
  const int shift = lineShift(placement);
  const int width =
      ceilDiv(area.lastColumn - area.firstColumn, placement.halfWidth);
  const int wide = ceilDiv(width * SCALE_ONE, PIXELS_PER_PHRASE_16 * MAX_SCALE);
  BitmapObject object;
  object.y =
      geometry.firstHalfLine + 2 * (placement.top + (area.firstRow >> shift));
  object.x = placement.left + divided(area.firstColumn, placement.halfWidth);
  object.depth = Depth::Bits16;
  object.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(phrases));
  object.pitch = 0;
  object.dataWidth = 1;
  object.imageWidth = wide;
  object.height = (area.lastRow - area.firstRow) >> shift;
  object.scaled = true;
  object.released = true;
  object.horizontalScale = static_cast<uint8_t>(
      ceilDiv(width * SCALE_ONE, PIXELS_PER_PHRASE_16 * wide));
  object.verticalScale = SCALE_ONE;
  return object;
}

void fillPhrase(uint8_t *phrase, uint16_t color) {
  const uint8_t high = static_cast<uint8_t>(color >> 8);
  const uint8_t low = static_cast<uint8_t>(color);
  const uint8_t bytes[sizeof(uint32_t)] = {high, low, high, low};
  uint32_t pair;
  std::memcpy(&pair, bytes, sizeof(pair));
  uint32_t *longs = reinterpret_cast<uint32_t *>(phrase);
  longs[0] = pair;
  longs[1] = pair;
}

void writeLineColors(const graphics::Layer &layer, const LayerArea &area,
                     const Placement &placement, uint8_t *target,
                     BuiltFrame &frame) {
  const int shift = lineShift(placement);
  const int lines = (area.lastRow - area.firstRow) >> shift;
  const uint16_t fallback = effectiveColor(layer, 0);
  const bool sameLines = sameRowColors(frame.lineRows, layer.rowColors);
  if (frame.lineTarget == target && frame.lineFirst == area.firstRow &&
      frame.lineCount == lines && frame.lineShift == shift &&
      frame.lineDefault == fallback && sameLines) {
    return;
  }
  const std::array<uint16_t, AMIGA_COLORS> &table = rgb16Table();
  for (int line = 0; line < lines; ++line) {
    fillPhrase(target + line * PHRASE_BYTES, table[fallback]);
  }
  const int odd = (1 << shift) - 1;
  for (const graphics::RowColor &change : layer.rowColors) {
    const int offset = change.row - area.firstRow;
    if (offset < 0 || (offset & odd) != 0) {
      continue;
    }
    const int line = offset >> shift;
    if (line < lines) {
      fillPhrase(target + line * PHRASE_BYTES,
                 table[change.color & COLOR_MASK]);
    }
  }
  std::copy_n(target + (lines - 1) * PHRASE_BYTES, PHRASE_BYTES,
              target + lines * PHRASE_BYTES);
  frame.lineRows = layer.rowColors;
  frame.lineTarget = target;
  frame.lineFirst = area.firstRow;
  frame.lineCount = lines;
  frame.lineShift = shift;
  frame.lineDefault = fallback;
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
  bool translated = false;
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
    if (!base || base->mask == FULL_MASK) {
      return;
    }
    for (int slot = 0; slot < CLUT_SIZE; ++slot) {
      const std::size_t index = static_cast<std::size_t>(slot);
      if (!m_used[index]) {
        colors[index] = effectiveColor(*base, slot);
      }
    }
  }

  const std::array<bool, CLUT_SIZE> &usedSlots() const { return m_used; }

private:
  std::array<uint16_t, CLUT_SIZE> m_colors{};
  std::array<bool, CLUT_SIZE> m_used{};
};

bool isEmpty(const LayerArea &area) {
  return area.firstRow >= area.lastRow || area.firstColumn >= area.lastColumn;
}

std::size_t sourceBytes(const graphics::Layer &layer) {
  return static_cast<std::size_t>(
      product(std::max(layer.stride, 0), std::max(layer.sourceRows, 0)));
}

bool isTranslatable(const graphics::Layer &layer) {
  const std::size_t bytes = sourceBytes(layer);
  return layer.rowColors.empty() && bytes != 0 &&
         bytes <= MAX_TRANSLATED_BYTES &&
         reinterpret_cast<uintptr_t>(layer.pixels) % LONG_ALIGNMENT == 0;
}

std::size_t baseLayer(const graphics::Display &display,
                      const std::vector<LayerArea> &areas) {
  std::size_t base = areas.size();
  int baseRows = 0;
  bool baseSprites = false;
  for (std::size_t index = 0; index < areas.size(); ++index) {
    const graphics::Layer &layer = display.layers[index];
    const LayerArea &area = areas[index];
    if (!layer.pixels || isEmpty(area)) {
      continue;
    }
    const int rows = area.lastRow - area.firstRow;
    if (layer.carriesSprites != baseSprites ? layer.carriesSprites
                                            : rows > baseRows) {
      base = index;
      baseRows = rows;
      baseSprites = layer.carriesSprites;
    }
  }
  return base;
}

bool onlyBackground(const graphics::RowColors &rows) {
  static graphics::RowColors checked;
  static bool background = false;
  if (rows.shares(checked)) {
    return background;
  }
  background = true;
  for (const graphics::RowColor &change : rows) {
    if (change.index != 0) {
      background = false;
      break;
    }
  }
  checked = rows;
  return background;
}

bool takesLineColors(const graphics::Layer &layer, const LayerPlan &plan) {
  if (layer.rowColors.empty() || !layer.pixels || layer.mask != FULL_MASK ||
      !plan.merged || plan.bank != 0 || plan.buffer) {
    return false;
  }
  return onlyBackground(layer.rowColors);
}

std::size_t planBanks(const graphics::Display &display,
                      const std::vector<LayerArea> &areas,
                      const FrameMemory &memory, std::vector<LayerPlan> &plans,
                      Banking &banking) {
  plans.assign(areas.size(), LayerPlan{});
  const std::size_t base = baseLayer(display, areas);
  if (base < areas.size()) {
    banking.claim(display.layers[base], 0);
    plans[base].merged = true;
  }
  for (std::size_t index = 0; index < areas.size(); ++index) {
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
        plan.translated = true;
        plan.bank = bank;
      }
      break;
    }
  }
  return base;
}

struct BankInput {
  bool pixels = false;
  bool empty = false;
  bool translatable = false;
  bool shared = false;
  uint8_t mask = FULL_MASK;
  int rows = 0;
  std::vector<uint16_t> palette;
};

struct BankCache {
  bool valid = false;
  uint32_t version = 0;
  uint32_t plan = 0;
  bool buffers = false;
  std::size_t base = 0;
  std::vector<BankInput> inputs;
  std::vector<LayerPlan> plans;
  std::vector<uint16_t> merged;
  std::array<uint16_t, CLUT_SIZE> clut{};
  std::array<bool, CLUT_SIZE> used{};
};

bool sameShape(const BankInput &input, const graphics::Layer &layer,
               const LayerArea &area) {
  return input.pixels == (layer.pixels != nullptr) &&
         input.empty == isEmpty(area) && input.mask == layer.mask &&
         input.rows == area.lastRow - area.firstRow &&
         input.translatable == isTranslatable(layer) &&
         input.palette.size() == layer.palette.size();
}

template <typename Visit>
void forClaims(const graphics::Layer &layer, const LayerPlan &plan,
               Visit visit) {
  if (!plan.merged || !layer.pixels) {
    return;
  }
  const uint32_t bank = plan.translated ? plan.bank : 0;
  for (std::size_t value = 0; value < layer.palette.size(); ++value) {
    if (isKept(layer, value)) {
      visit(value ^ bank, layer.palette[value] & COLOR_MASK);
    }
  }
}

BankCache &bankCache() {
  static BankCache banks;
  return banks;
}

std::vector<LayerPlan> &layerPlans() {
  static std::vector<LayerPlan> plans;
  return plans;
}

bool takePlans(const BankCache &banks, const graphics::Display &display,
               const FrameMemory &memory, bool buffers,
               std::vector<LayerPlan> &plans) {
  plans = banks.plans;
  for (std::size_t index = 0; index < plans.size(); ++index) {
    LayerPlan &plan = plans[index];
    plan.buffer = nullptr;
    if (plan.translated && buffers) {
      const graphics::Layer &layer = display.layers[index];
      plan.buffer = memory.buffers->buffer(layer.pixels, sourceBytes(layer));
      if (!plan.buffer) {
        return false;
      }
    }
  }
  return true;
}

void rememberBanks(BankCache &banks, const graphics::Display &display,
                   const std::vector<LayerArea> &areas,
                   const FrameMemory &memory,
                   const std::vector<LayerPlan> &plans, const Banking &banking,
                   std::size_t base) {
  banks.valid = true;
  ++banks.version;
  ++banks.plan;
  banks.buffers = memory.buffers != nullptr;
  banks.base = base;
  banks.used = banking.usedSlots();
  banks.inputs.resize(areas.size());
  for (std::size_t index = 0; index < areas.size(); ++index) {
    const graphics::Layer &layer = display.layers[index];
    const LayerArea &area = areas[index];
    BankInput &input = banks.inputs[index];
    input.pixels = layer.pixels != nullptr;
    input.empty = isEmpty(area);
    input.translatable = isTranslatable(layer);
    input.mask = layer.mask;
    input.rows = area.lastRow - area.firstRow;
    input.palette = layer.palette;
  }
  banks.plans = plans;
  std::array<uint8_t, CLUT_SIZE> claims{};
  for (std::size_t index = 0; index < areas.size(); ++index) {
    const LayerPlan &plan = plans[index];
    if (plan.solidSlot != NO_SLOT) {
      ++claims[static_cast<std::size_t>(plan.solidSlot)];
    }
    forClaims(display.layers[index], plan,
              [&claims](std::size_t slot, uint16_t) { ++claims[slot]; });
  }
  for (std::size_t index = 0; index < areas.size(); ++index) {
    bool shared = false;
    forClaims(display.layers[index], plans[index],
              [&claims, &shared](std::size_t slot, uint16_t) {
                shared = shared || claims[slot] > 1;
              });
    banks.inputs[index].shared = shared;
  }
  banking.palette(base < areas.size() ? &display.layers[base] : nullptr,
                  banks.merged);
  const std::array<uint16_t, AMIGA_COLORS> &table = rgb16Table();
  for (int value = 0; value < CLUT_SIZE; ++value) {
    const std::size_t index = static_cast<std::size_t>(value);
    banks.clut[index] = table[banks.merged[index] & COLOR_MASK];
  }
}

bool samePlan(const LayerPlan &left, const LayerPlan &right) {
  return left.merged == right.merged && left.translated == right.translated &&
         left.bank == right.bank && left.solidSlot == right.solidSlot;
}

void recolorChanged(BankCache &banks, const graphics::Display &display,
                    const std::vector<LayerPlan> &plans) {
  const std::array<uint16_t, AMIGA_COLORS> &table = rgb16Table();
  const auto recolor = [&banks, &table](std::size_t slot, uint16_t color) {
    banks.merged[slot] = color;
    banks.clut[slot] = table[color];
  };
  bool changed = false;
  for (std::size_t index = 0; index < plans.size(); ++index) {
    const graphics::Layer &layer = display.layers[index];
    BankInput &input = banks.inputs[index];
    const LayerPlan &plan = plans[index];
    const bool claims = plan.merged && layer.pixels;
    const uint32_t bank = plan.translated ? plan.bank : 0;
    const bool fills = index == banks.base && layer.mask != FULL_MASK;
    const std::size_t step = static_cast<std::size_t>(layer.mask) + 1;
    const bool stepped = (layer.mask & step) == 0;
    bool refill = false;
    for (std::size_t value = 0; value < layer.palette.size(); ++value) {
      if (input.palette[value] == layer.palette[value] ||
          !isKept(layer, value)) {
        continue;
      }
      changed = true;
      const uint16_t color = layer.palette[value] & COLOR_MASK;
      if (claims) {
        recolor(value ^ bank, color);
      }
      if (fills && stepped) {
        for (std::size_t slot = value; slot < banks.used.size(); slot += step) {
          if (!banks.used[slot]) {
            recolor(slot, color);
          }
        }
      }
      refill = refill || (fills && !stepped);
    }
    if (refill) {
      for (std::size_t slot = 0; slot < banks.used.size(); ++slot) {
        if (!banks.used[slot]) {
          recolor(slot, effectiveColor(layer, static_cast<int>(slot)));
        }
      }
    }
    input.palette = layer.palette;
  }
  if (changed) {
    ++banks.version;
  }
}

bool cachedBanks(BankCache &banks, const graphics::Display &display,
                 const std::vector<LayerArea> &areas, const FrameMemory &memory,
                 bool buffers, std::vector<LayerPlan> &plans) {
  if (!banks.valid || banks.buffers != (memory.buffers != nullptr) ||
      banks.inputs.size() != areas.size() ||
      banks.merged.size() != banks.clut.size() ||
      baseLayer(display, areas) != banks.base) {
    return false;
  }
  bool recolored = false;
  for (std::size_t index = 0; index < areas.size(); ++index) {
    const graphics::Layer &layer = display.layers[index];
    const BankInput &input = banks.inputs[index];
    if (!sameShape(input, layer, areas[index])) {
      return false;
    }
    if (input.palette != layer.palette) {
      if (input.shared || !layer.pixels) {
        return false;
      }
      recolored = true;
    }
  }
  if (!takePlans(banks, display, memory, buffers, plans)) {
    return false;
  }
  if (recolored) {
    recolorChanged(banks, display, plans);
  }
  return true;
}

bool recolorBanks(BankCache &banks, const graphics::Display &display,
                  const std::vector<LayerArea> &areas,
                  const FrameMemory &memory,
                  const std::vector<LayerPlan> &plans, std::size_t base) {
  if (!banks.valid || banks.base != base ||
      banks.buffers != (memory.buffers != nullptr) ||
      banks.inputs.size() != areas.size() ||
      banks.plans.size() != plans.size() ||
      banks.merged.size() != banks.clut.size()) {
    return false;
  }
  for (std::size_t index = 0; index < areas.size(); ++index) {
    const graphics::Layer &layer = display.layers[index];
    const BankInput &input = banks.inputs[index];
    if (!samePlan(plans[index], banks.plans[index]) ||
        !sameShape(input, layer, areas[index]) ||
        (!layer.pixels && input.palette != layer.palette)) {
      return false;
    }
  }
  recolorChanged(banks, display, plans);
  return true;
}

void settleBanks(BankCache &banks, const graphics::Display &display,
                 const std::vector<LayerArea> &areas, const FrameMemory &memory,
                 bool buffers, std::vector<LayerPlan> &plans) {
  if (cachedBanks(banks, display, areas, memory, buffers, plans)) {
    return;
  }
  Banking banking;
  const std::size_t base = planBanks(display, areas, memory, plans, banking);
  if (!recolorBanks(banks, display, areas, memory, plans, base)) {
    rememberBanks(banks, display, areas, memory, plans, banking, base);
  }
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
  placement.left =
      (geometry.columns - divided(display.width, placement.halfWidth)) / 2;
  placement.top =
      (geometry.rows - divided(display.height, placement.rowsPerLine)) / 2;
  return placement;
}

LayerArea visibleArea(const graphics::Display &display,
                      const graphics::Layer &layer, const Placement &placement,
                      const Geometry &geometry) {
  const Rows rows = sourceRows(layer);
  const Rows columns = sourceColumns(layer);
  const int perLine = placement.rowsPerLine;
  LayerArea area;
  area.firstRow = alignUp(
      std::max({0, rows.first, scaled(-placement.top, perLine)}), perLine);
  area.lastRow =
      alignUp(std::min({display.height, rows.last,
                        scaled(geometry.rows - placement.top, perLine)}),
              perLine);
  area.firstColumn = std::max(
      {0, columns.first, scaled(-placement.left, placement.halfWidth)});
  area.lastColumn = std::min(
      {display.width, columns.last,
       scaled(geometry.columns - placement.left, placement.halfWidth)});
  if (area.lastRow < area.firstRow) {
    area.lastRow = area.firstRow;
  }
  if (area.lastColumn < area.firstColumn) {
    area.lastColumn = area.firstColumn;
  }
  return area;
}

bool sameLayout(const graphics::Display &left, const graphics::Display &right) {
  return sameLayers(left, right) && sameSprites(left, right);
}

bool sameSprites(const graphics::Display &left,
                 const graphics::Display &right) {
  if (left.layers.size() != right.layers.size()) {
    return false;
  }
  const graphics::Layer *other = right.layers.data();
  for (const graphics::Layer &a : left.layers) {
    if (a.sprites != (*other++).sprites) {
      return false;
    }
  }
  return true;
}

bool sameSprites(const graphics::Display &display, const SpriteLists &sprites) {
  if (display.layers.size() != sprites.size()) {
    return false;
  }
  const std::vector<graphics::Sprite> *other = sprites.data();
  for (const graphics::Layer &layer : display.layers) {
    if (layer.sprites != *other++) {
      return false;
    }
  }
  return true;
}

bool samePlacing(const graphics::Display &left,
                 const graphics::Display &right) {
  auto a = left.layers.begin();
  auto b = right.layers.begin();
  for (; a != left.layers.end() && b != right.layers.end(); ++a, ++b) {
    if (a->pixels != b->pixels || a->sourceX != b->sourceX ||
        a->sourceY != b->sourceY || a->stride != b->stride ||
        a->sourceColumns != b->sourceColumns ||
        a->sourceRows != b->sourceRows || a->sourceStep != b->sourceStep ||
        a->repeat != b->repeat || a->wrap != b->wrap || a->left != b->left ||
        a->top != b->top || a->columns != b->columns || a->rows != b->rows ||
        a->mask != b->mask || a->carriesSprites != b->carriesSprites) {
      return false;
    }
  }
  return a == left.layers.end() && b == right.layers.end() &&
         left.width == right.width && left.height == right.height &&
         left.displayHeight == right.displayHeight &&
         left.border == right.border;
}

bool sameColors(const graphics::Display &left, const graphics::Display &right) {
  const graphics::Layer *other = right.layers.data();
  for (const graphics::Layer &a : left.layers) {
    const graphics::Layer &b = *other++;
    if (a.palette != b.palette || !sameRowColors(a.rowColors, b.rowColors)) {
      return false;
    }
  }
  return true;
}

bool sameLayers(const graphics::Display &left, const graphics::Display &right) {
  return samePlacing(left, right) && sameColors(left, right);
}

void allowCopper(bool allowed) { copperAllowed = allowed; }

namespace {

void addMasks(const graphics::Display &display, const Placement &placement,
              const Geometry &geometry, uint8_t *phrase, uint16_t color,
              ObjectList &list, std::vector<int> &masks) {
  fillPhrase(phrase, color);
  const int firstLine = std::max(placement.top, 0);
  const int lastLine =
      std::min(geometry.rows,
               placement.top + divided(display.height, placement.rowsPerLine));
  if (firstLine >= lastLine) {
    return;
  }
  BitmapObject mask;
  mask.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(phrase));
  mask.depth = Depth::Bits16;
  mask.pitch = 0;
  mask.dataWidth = 0;
  mask.imageWidth = MASK_PIXELS / PIXELS_PER_PHRASE_16;
  mask.y = geometry.firstHalfLine + 2 * firstLine;
  mask.height = lastLine - firstLine;
  mask.x = placement.left - MASK_PIXELS;
  masks.push_back(static_cast<int>(list.addBitmap(mask)));
  mask.x = placement.left + divided(display.width, placement.halfWidth);
  masks.push_back(static_cast<int>(list.addBitmap(mask)));
}

void addSprites(const graphics::Layer &layer, std::size_t index,
                const LayerArea &area, const Placement &placement,
                const Geometry &geometry, uint32_t solidPhrase,
                ObjectList &list, BuiltFrame &frame) {
  SpriteSlots slots;
  slots.layer = index;
  for (std::size_t slot = 0; slot < graphics::SPRITE_SLOTS; ++slot) {
    const BitmapObject object =
        slot < layer.sprites.size()
            ? spriteObject(layer, layer.sprites[slot], area, placement,
                           geometry, solidPhrase)
            : parkedObject(geometry, solidPhrase, placement);
    slots.objects[slot] = static_cast<int>(list.addBitmap(object));
  }
  frame.spriteSlots.push_back(slots);
}

} // namespace

void buildFrame(const graphics::Display &display, const Geometry &geometry,
                const FrameMemory &memory, const Overlay *overlays,
                std::size_t overlayCount, BuiltFrame &frame) {
  const Placement placement = placeDisplay(display, geometry);
  static std::vector<LayerArea> areas;
  areas.clear();
  for (const graphics::Layer &layer : display.layers) {
    areas.push_back(visibleArea(display, layer, placement, geometry));
  }

  std::vector<LayerPlan> &plans = layerPlans();
  BankCache &banks = bankCache();
  settleBanks(banks, display, areas, memory, true, plans);
  frame.plan = banks.plan;

  frame.translations.clear();
  static std::vector<const uint8_t *> pixels;
  pixels.clear();
  const std::size_t count = areas.size();
  frame.layerTranslations.resize(count);
  std::size_t lineLayer = count;
  uint8_t *lines = nullptr;
  bool complete = true;
  for (std::size_t index = 0; index < count; ++index) {
    const LayerPlan &plan = plans[index];
    const graphics::Layer &layer = display.layers[index];
    pixels.push_back(plan.buffer ? plan.buffer : layer.pixels);
    frame.layerTranslations[index] = NO_OBJECT;
    if (plan.buffer) {
      frame.layerTranslations[index] =
          static_cast<int>(frame.translations.size());
      frame.translations.push_back({layer.pixels, plan.buffer,
                                    sourceBytes(layer), everyByte(layer.mask),
                                    everyByte(plan.bank)});
    }
    const LayerArea &area = areas[index];
    if (!lines && takesLineColors(layer, plan)) {
      lines = linePhrases(
          memory, ((area.lastRow - area.firstRow) >> lineShift(placement)) + 1);
      if (lines) {
        lineLayer = index;
        writeLineColors(layer, area, placement, lines, frame);
      }
    }
    if (!isEmpty(area) &&
        (!plan.merged || (!layer.rowColors.empty() && index != lineLayer))) {
      complete = false;
    }
  }

  frame.background = rgb16Table()[display.border & COLOR_MASK];
  frame.border = borderColor(display.border);
  if (complete) {
    if (frame.clutVersion != banks.version) {
      frame.clut = banks.clut;
      frame.clutVersion = banks.version;
    }
    frame.copper.assign(1, COPPER_END);
  } else {
    frame.clutVersion = 0;
    static graphics::Display adjusted;
    adjusted = display;
    const bool combined =
        std::count_if(plans.begin(), plans.end(),
                      [](const LayerPlan &plan) { return plan.merged; }) > 1;
    for (std::size_t index = 0; index < count; ++index) {
      const LayerPlan &plan = plans[index];
      graphics::Layer &layer = adjusted.layers[index];
      if (plan.merged) {
        layer.palette = banks.merged;
        if (combined) {
          layer.mask = FULL_MASK;
        }
      }
      layer.pixels = pixels[index];
      if (index == lineLayer) {
        layer.rowColors.clear();
      }
    }
    buildPalettes(adjusted, geometry, placement, areas, frame);
    if (!copperAllowed) {
      frame.copper.assign(1, COPPER_END);
    }
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
  frame.placement = placement;
  frame.areas.assign(areas.begin(), areas.end());
  frame.objects.assign(count, NO_OBJECT);
  frame.lineObject = NO_OBJECT;
  frame.lineLayer = NO_OBJECT;
  frame.spriteSlots.clear();
  frame.masks.clear();
  for (std::size_t index = 0; index < count; ++index) {
    if (index == lineLayer) {
      frame.lineObject = static_cast<int>(
          list.addBitmap(lineObject(areas[index], placement, geometry, lines)));
      frame.lineLayer = static_cast<int>(index);
    }
    const int slot =
        plans[index].solidSlot == NO_SLOT ? 0 : plans[index].solidSlot;
    frame.objects[index] = addObject(
        display.layers[index], pixels[index], areas[index], placement, geometry,
        memory.solidPhrases + static_cast<uint32_t>(slot) * PHRASE_BYTES,
        index == lineLayer, list);
    if (display.layers[index].carriesSprites && !plans[index].translated) {
      addSprites(display.layers[index], index, areas[index], placement,
                 geometry, memory.solidPhrases, list, frame);
    }
  }
  if (!frame.spriteSlots.empty() && memory.maskPhrase) {
    addMasks(display, placement, geometry, memory.maskPhrase, frame.background,
             list, frame.masks);
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

bool scrollsOnly(const graphics::Display &left,
                 const graphics::Display &right) {
  if (left.width != right.width || left.height != right.height ||
      left.displayHeight != right.displayHeight ||
      left.border != right.border ||
      left.layers.size() != right.layers.size()) {
    return false;
  }
  const graphics::Layer *other = right.layers.data();
  for (const graphics::Layer &a : left.layers) {
    const graphics::Layer &b = *other++;
    if ((a.pixels == nullptr) != (b.pixels == nullptr) ||
        a.stride != b.stride || a.sourceColumns != b.sourceColumns ||
        a.sourceRows != b.sourceRows || a.sourceStep != b.sourceStep ||
        a.repeat != b.repeat || a.wrap != b.wrap || a.left != b.left ||
        a.top != b.top || a.columns != b.columns || a.rows != b.rows ||
        a.mask != b.mask || a.carriesSprites != b.carriesSprites ||
        a.palette.size() != b.palette.size() ||
        !sameRowColors(a.rowColors, b.rowColors)) {
      return false;
    }
  }
  return true;
}

bool samePalettes(const graphics::Display &left,
                  const graphics::Display &right) {
  const graphics::Layer *other = right.layers.data();
  for (const graphics::Layer &layer : left.layers) {
    if (layer.palette != (other++)->palette) {
      return false;
    }
  }
  return true;
}

namespace {

bool patchSprites(const graphics::Display &display,
                  const graphics::Display &built, const SpriteLists *wanted,
                  const std::vector<LayerArea> &areas,
                  const Placement &placement, const Geometry &geometry,
                  uint32_t solidPhrase, BuiltFrame &frame) {
  for (const SpriteSlots &slots : frame.spriteSlots) {
    const graphics::Layer &layer = display.layers[slots.layer];
    const graphics::Layer &old = built.layers[slots.layer];
    const std::vector<graphics::Sprite> &sprites =
        wanted ? (*wanted)[slots.layer] : layer.sprites;
    const LayerArea &area = areas[slots.layer];
    const bool moved = layer.sourceX != old.sourceX ||
                       layer.sourceY != old.sourceY ||
                       !sameArea(area, frame.areas[slots.layer]);
    const std::size_t count = sprites.size();
    const std::size_t oldCount = old.sprites.size();
    const bool fast = hasFastSprites(layer, placement);
    for (std::size_t slot = 0; slot < graphics::SPRITE_SLOTS; ++slot) {
      const bool shown = slot < count;
      const bool wasShown = slot < oldCount;
      if (!shown && !wasShown) {
        continue;
      }
      if (!moved && shown && wasShown && sprites[slot] == old.sprites[slot]) {
        continue;
      }
      uint64_t *phrases =
          frame.phrases.data() + static_cast<std::size_t>(slots.objects[slot]);
      SpriteFields fields;
      if (shown && fast && !isScaledBitmap(phrases[0]) &&
          isPlainSprite(sprites[slot]) &&
          fastSprite(layer, sprites[slot], area, placement, geometry, fields)) {
        rewriteSprite(phrases, fields.data, fields.x, fields.y, fields.height,
                      fields.dataWidth, fields.imageWidth, fields.firstPixel);
        continue;
      }
      const BitmapObject object =
          shown ? spriteObject(layer, sprites[slot], area, placement, geometry,
                               solidPhrase)
                : parkedObject(geometry, solidPhrase, placement);
      if (isScaledBitmap(phrases[0]) != object.scaled) {
        return false;
      }
      rewriteBitmap(object, phrases);
    }
  }
  return true;
}

bool samePlacement(const Placement &left, const Placement &right) {
  return left.left == right.left && left.top == right.top &&
         left.halfWidth == right.halfWidth &&
         left.rowsPerLine == right.rowsPerLine;
}

} // namespace

bool copyFrame(const BuiltFrame &from, const FrameMemory &memory,
               BuiltFrame &to) {
  if (from.lineObject != NO_OBJECT ||
      (!from.masks.empty() && !memory.maskPhrase)) {
    return false;
  }
  to = from;
  to.lineTarget = nullptr;
  if (!to.masks.empty()) {
    fillPhrase(memory.maskPhrase, to.background);
    const uint32_t data =
        static_cast<uint32_t>(reinterpret_cast<uintptr_t>(memory.maskPhrase));
    for (const int mask : to.masks) {
      retargetBitmap(to.phrases.data() + mask, data);
    }
  }
  return true;
}

bool scrollFrame(const graphics::Display &display,
                 const graphics::Display &built, const Geometry &geometry,
                 const FrameMemory &memory, BuiltFrame &frame) {
  const std::size_t count = frame.objects.size();
  if (count != frame.areas.size() || count != frame.layerTranslations.size() ||
      frame.copper.size() > 1 || !scrollsOnly(display, built)) {
    return false;
  }
  const Placement placement = placeDisplay(display, geometry);
  if (!samePlacement(placement, frame.placement)) {
    return false;
  }
  if (display.layers.size() != count) {
    return false;
  }
  static std::vector<LayerArea> areas;
  areas.clear();
  const graphics::Layer *previous = built.layers.data();
  bool rowsMoved = false;
  for (const graphics::Layer &layer : display.layers) {
    const graphics::Layer &before = *previous++;
    const LayerArea &old = frame.areas[areas.size()];
    if (layer.sourceX == before.sourceX && layer.sourceY == before.sourceY &&
        layer.sourceColumns == before.sourceColumns &&
        layer.sourceRows == before.sourceRows) {
      areas.push_back(old);
      continue;
    }
    areas.push_back(visibleArea(display, layer, placement, geometry));
    const LayerArea &now = areas.back();
    if (isEmpty(now) != isEmpty(old)) {
      return false;
    }
    rowsMoved =
        rowsMoved || now.firstRow != old.firstRow || now.lastRow != old.lastRow;
  }
  if (rowsMoved &&
      (frame.clutVersion == 0 || frame.lineLayer != NO_OBJECT ||
       baseLayer(display, areas) != baseLayer(built, frame.areas))) {
    return false;
  }
  const graphics::Layer *old = built.layers.data();
  for (std::size_t index = 0; index < count; ++index, ++old) {
    const graphics::Layer &layer = display.layers[index];
    if (frame.objects[index] == NO_OBJECT || !layer.pixels) {
      continue;
    }
    if (layer.sourceX == old->sourceX && layer.sourceY == old->sourceY &&
        layer.pixels == old->pixels &&
        sameArea(areas[index], frame.areas[index])) {
      continue;
    }
    const uint8_t *pixels = layer.pixels;
    const int translated = frame.layerTranslations[index];
    if (translated != NO_OBJECT) {
      Translation &translation =
          frame.translations[static_cast<std::size_t>(translated)];
      if (translation.source != layer.pixels) {
        if (!memory.buffers || !isTranslatable(layer)) {
          return false;
        }
        uint8_t *buffer =
            memory.buffers->buffer(layer.pixels, translation.bytes);
        if (!buffer) {
          return false;
        }
        translation.source = layer.pixels;
        translation.target = buffer;
      }
      pixels = translation.target;
    }
    BitmapObject object;
    if (!makeObject(layer, pixels, areas[index], placement, geometry, 0,
                    static_cast<int>(index) == frame.lineLayer, object)) {
      return false;
    }
    uint64_t *phrases =
        frame.phrases.data() + static_cast<std::size_t>(frame.objects[index]);
    if (isScaledBitmap(phrases[0]) != object.scaled) {
      return false;
    }
    rewriteBitmap(object, phrases);
  }
  if (frame.lineLayer != NO_OBJECT) {
    const std::size_t index = static_cast<std::size_t>(frame.lineLayer);
    const LayerArea &area = areas[index];
    uint8_t *lines = linePhrases(
        memory, ((area.lastRow - area.firstRow) >> lineShift(placement)) + 1);
    if (!lines) {
      return false;
    }
    writeLineColors(display.layers[index], area, placement, lines, frame);
    rewriteBitmap(lineObject(area, placement, geometry, lines),
                  frame.phrases.data() +
                      static_cast<std::size_t>(frame.lineObject));
  }
  if (!patchSprites(display, built, nullptr, areas, placement, geometry,
                    memory.solidPhrases, frame)) {
    return false;
  }
  frame.areas.assign(areas.begin(), areas.end());
  return samePalettes(display, built) || recolorPalettes(display, frame) ||
         recolorFrame(display, display, geometry, memory, frame);
}

bool recolorFrame(const graphics::Display &display,
                  const graphics::Display &built, const Geometry &geometry,
                  const FrameMemory &memory, BuiltFrame &frame) {
  if (frame.clutVersion == 0 ||
      frame.layerTranslations.size() != display.layers.size() ||
      frame.areas.size() != display.layers.size() ||
      !samePlacing(display, built) ||
      !samePlacement(placeDisplay(display, geometry), frame.placement)) {
    return false;
  }
  const graphics::Layer *other = built.layers.data();
  for (const graphics::Layer &layer : display.layers) {
    const graphics::Layer &before = *other++;
    if (layer.palette.size() != before.palette.size() ||
        !layer.rowColors.empty() || !before.rowColors.empty()) {
      return false;
    }
  }
  const std::vector<LayerArea> &areas = frame.areas;
  std::vector<LayerPlan> &plans = layerPlans();
  BankCache &banks = bankCache();
  settleBanks(banks, display, areas, memory, false, plans);
  for (std::size_t index = 0; index < areas.size(); ++index) {
    const LayerPlan &plan = plans[index];
    const int used = frame.layerTranslations[index];
    if (isEmpty(areas[index])) {
      continue;
    }
    if (!display.layers[index].pixels || !plan.merged) {
      return false;
    }
    if (!plan.translated) {
      if (used != NO_OBJECT || plan.bank != 0) {
        return false;
      }
      continue;
    }
    if (used == NO_OBJECT) {
      return false;
    }
    const graphics::Layer &layer = display.layers[index];
    const Translation &translation =
        frame.translations[static_cast<std::size_t>(used)];
    if (translation.source != layer.pixels ||
        translation.keep != everyByte(layer.mask) ||
        translation.flip != everyByte(plan.bank)) {
      return false;
    }
  }
  frame.clut = banks.clut;
  frame.clutVersion = banks.version;
  frame.plan = banks.plan;
  return true;
}

bool recolorPalettes(const graphics::Display &display, BuiltFrame &frame) {
  BankCache &banks = bankCache();
  if (frame.clutVersion == 0 || !banks.valid || frame.plan != banks.plan ||
      banks.inputs.size() != display.layers.size() ||
      banks.plans.size() != display.layers.size() ||
      banks.merged.size() != banks.clut.size()) {
    return false;
  }
  for (std::size_t index = 0; index < display.layers.size(); ++index) {
    const graphics::Layer &layer = display.layers[index];
    const BankInput &input = banks.inputs[index];
    if (!layer.rowColors.empty() ||
        input.palette.size() != layer.palette.size() ||
        ((input.shared || !layer.pixels) && input.palette != layer.palette)) {
      return false;
    }
  }
  recolorChanged(banks, display, banks.plans);
  frame.clut = banks.clut;
  frame.clutVersion = banks.version;
  return true;
}

bool moveSprites(const graphics::Display &display,
                 const graphics::Display &built, const Geometry &geometry,
                 const FrameMemory &memory, BuiltFrame &frame) {
  return frame.areas.size() == display.layers.size() &&
         patchSprites(display, built, nullptr, frame.areas, frame.placement,
                      geometry, memory.solidPhrases, frame);
}

bool moveSprites(const graphics::Display &built, const SpriteLists &sprites,
                 const Geometry &geometry, const FrameMemory &memory,
                 BuiltFrame &frame) {
  return frame.areas.size() == built.layers.size() &&
         sprites.size() == built.layers.size() &&
         patchSprites(built, built, &sprites, frame.areas, frame.placement,
                      geometry, memory.solidPhrases, frame);
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
