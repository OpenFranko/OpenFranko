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

  struct ExposedColumn {
    int x = 0;
    int column = 0;
    int lines = 0;
  };

  enum class Update { None, Resave, Spans, Panned, Full };

  int pannedLayer(const Display &display) const;
  void panLayer(const Display &display, std::size_t index);
  void arrange(const Display &display, bool resized);
  void placeBlock(const Layer &layer, Mapping &mapping);
  Mapping &mapped(const Layer &layer, Mapping &mapping);
  uint8_t slot(uint16_t color);
  uint8_t recolorSlot(uint16_t color);
  void remember(uint16_t color, uint8_t slot);
  uint8_t nearestSlot(uint16_t color) const;
  void assignRecolors(const Display &display);
  bool keepsColors(const Display &display) const;
  Drawn drawn(const Layer &layer, Mapping &mapping);
  void place(const Display &display);
  bool isShown(std::size_t index, int row) const;
  bool isPlain(const Display &display, std::size_t index, int row) const;
  bool isRecolored(std::size_t index, int row) const;
  void findChanges(const Display &display);
  void moveRows(std::size_t index);
  void panRows(const Display &display, std::size_t index, int moved);
  void compareWindow(const Display &display, std::size_t index, int row);
  void markSprites(const Display &display);
  int pinSprite(const Display &display, std::size_t index,
                const std::vector<Sprite> &before);
  void markSprite(const Display &display, std::size_t index,
                  const Sprite &sprite, bool kept, bool old);
  static Span spriteColumns(const Layer &layer, const Placed &placed,
                            const Sprite &sprite, bool wrapped);
  Span changedSpan(const Display &display, std::size_t index, int row, int from,
                   int to) const;
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
  void drawExposed(const Display &display);
  void drawWindow(const Display &display, std::size_t index, int row, int from,
                  int to);
  const std::array<uint8_t, FRAME_COLORS> &rowSlots(const Layer &layer,
                                                    std::size_t index, int row);
  void overlaySpans(const Display &display, int row);
  void drawPinned(const Display &display, std::size_t index, int row);
  void overlaySprites(const Display &display, std::size_t index, int row,
                      int from, int to,
                      const std::array<uint8_t, FRAME_COLORS> &slots);
  uint8_t drawSpan(const Layer &layer, const Placed &placed,
                   const Mapping &shown, int row, int from, int to);
  void drawChecked(const Layer &layer, const Placed &placed, Mapping &mapping,
                   int row, int from, int to);
  void recolor(const Layer &layer, std::size_t index, int row);
  void uncolor(const Layer &layer, std::size_t index, int row,
               const Mapping &base);
  void save(const Display &display, int row);
  void saveRow(const Layer &layer, std::size_t index, int sourceRow);
  void saveWindow(const Display &display, std::size_t index, int row, int from,
                  int to);

  Scan m_leading;
  Scan m_trailing;
  IndexedFrame *m_frame = nullptr;
  const IndexedFrame *m_lastFrame = nullptr;
  std::vector<int16_t> m_colorSlots;
  std::vector<uint16_t> m_assignedColors;
  std::vector<int16_t> m_recolorSlots;
  std::vector<uint16_t> m_recolorColors;
  bool m_colorsKept = false;
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
  std::vector<int> m_onlyLayers;
  std::vector<std::vector<int>> m_recolorStarts;
  std::vector<std::vector<int>> m_recolorOrder;
  std::vector<std::vector<uint8_t>> m_recolorTargets;
  std::vector<int> m_recolorNext;
  std::vector<std::vector<uint8_t>> m_saved;
  std::vector<Update> m_updates;
  Mapping m_rowMapping;
  int m_rowLayer = -1;
  int m_exposedLayer = -1;
  std::vector<bool> m_sourceKept;
  bool m_anyKept = false;
  bool m_anySprites = false;
  int m_pannedLayer = -1;
  int m_pannedBy = 0;
  int m_pinnedSprite = -1;
  int m_pinnedX = 0;
  Span m_pinnedSpan;
  std::vector<bool> m_pinnedRows;
  std::vector<ExposedColumn> m_exposedColumns;
  std::vector<bool> m_recoloredRows;
  Span m_exposed;
};

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_INDEXEDRASTERIZER_H_
