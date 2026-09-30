#ifndef SYSTEMS_GRAPHICS_INDEXEDRASTERIZER_H_
#define SYSTEMS_GRAPHICS_INDEXEDRASTERIZER_H_

#include "graphics/Display.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {

inline constexpr std::size_t FRAME_COLORS = 256;
inline constexpr int SHIFT_STEP = 8;
inline constexpr int NO_ROW = -1;

struct Span {
  int first = 0;
  int last = 0;
};

struct RowChange {
  int from = NO_ROW;
  int shift = 0;
  std::array<Span, 2> spans{};
};

bool isChanged(const RowChange &change);

struct IndexedFrame {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> pixels;
  std::array<uint16_t, FRAME_COLORS> palette{};
  std::vector<RowChange> changes;
};

std::size_t leadingBytes(const uint8_t *left, const uint8_t *right,
                         std::size_t count);
std::size_t trailingBytes(const uint8_t *left, const uint8_t *right,
                          std::size_t count);

class IndexedRasterizer {
public:
  using Scan = std::size_t (*)(const uint8_t *left, const uint8_t *right,
                               std::size_t count);

  explicit IndexedRasterizer(Scan leading = leadingBytes,
                             Scan trailing = trailingBytes);

  void rasterize(const Display &display, IndexedFrame &frame);

private:
  struct Mapping {
    std::array<uint8_t, FRAME_COLORS> slots{};
    bool mapped = false;
    bool placed = false;
    bool block = false;
    bool checked = false;
    uint8_t keep = 0;
    uint8_t base = 0;
  };

  struct Drawn {
    const uint8_t *pixels = nullptr;
    int stride = 0;
    int sourceColumns = 0;
    int sourceRows = 0;
    int sourceX = 0;
    int sourceY = 0;
    int sourceStep = 0;
    int repeat = 0;
    bool wrap = false;
    int left = 0;
    int top = 0;
    int columns = 0;
    int rows = 0;
    uint8_t mask = 0;
    bool block = false;
    bool checked = false;
    uint8_t keep = 0;
    uint8_t base = 0;
    std::array<uint8_t, FRAME_COLORS> slots{};
  };

  struct Placed {
    int firstRow = 0;
    int lastRow = 0;
    int first = 0;
    int last = 0;
    int start = 0;
    int end = 0;
    int wrapEnd = 0;
    int shift = 0;
  };

  enum class Update { None, Resave, Spans, Full };

  bool isUnchanged(const Display &display) const;
  void arrange(const Display &display, bool resized);
  void placeBlock(const Layer &layer, Mapping &mapping);
  Mapping &mapped(const Layer &layer, Mapping &mapping);
  uint8_t slot(uint16_t color);
  void remember(uint16_t color, uint8_t slot);
  uint8_t nearestSlot(uint16_t color) const;
  Drawn drawn(const Layer &layer, Mapping &mapping);
  void place(const Display &display);
  bool isShown(std::size_t index, int row) const;
  bool isPlain(const Display &display, std::size_t index, int row) const;
  bool isRecolored(std::size_t index, int row) const;
  void findChanges(const Display &display);
  void moveRows(std::size_t index);
  void compareRows(const Display &display, std::size_t index, int firstRow,
                   int lastRow);
  void compareRow(const Display &display, std::size_t index, int row,
                  bool differs);
  RowChange shifted(const uint8_t *in, const uint8_t *saved, int count,
                    int shift) const;
  bool hasChanged(const Display &display, int row) const;
  void drawRow(const Display &display, int row, uint8_t border);
  void shiftRows(int firstRow, int lastRow, int shift);
  void drawSpans(const Display &display, int row);
  uint8_t drawSpan(const Layer &layer, const Placed &placed,
                   const Mapping &shown, int row, int from, int to);
  void drawChecked(const Layer &layer, const Placed &placed, Mapping &mapping,
                   int row, int from, int to);
  void recolor(const Layer &layer, int row);
  void save(const Display &display, int row);
  void saveRow(const Layer &layer, std::size_t index, int sourceRow);

  Scan m_leading;
  Scan m_trailing;
  IndexedFrame *m_frame = nullptr;
  const IndexedFrame *m_lastFrame = nullptr;
  std::vector<int16_t> m_colorSlots;
  std::vector<uint16_t> m_assignedColors;
  std::array<bool, FRAME_COLORS> m_used{};
  std::size_t m_nextFree = 0;
  std::vector<Mapping> m_mappings;
  std::vector<Drawn> m_drawn;
  std::vector<Drawn> m_lastDrawn;
  uint8_t m_border = 0;
  int m_lastBorder = -1;
  Display m_lastDisplay;
  int m_lastShift = -SHIFT_STEP;
  int m_streak = 0;
  std::vector<Placed> m_placed;
  std::vector<int> m_topLayers;
  std::vector<int> m_soleLayers;
  std::vector<int> m_lastSoleLayers;
  std::vector<std::vector<bool>> m_recolored;
  std::vector<std::vector<uint8_t>> m_saved;
  std::vector<Update> m_updates;
  Mapping m_rowMapping;
};

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_INDEXEDRASTERIZER_H_
