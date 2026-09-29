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

struct IndexedFrame {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> pixels;
  std::array<uint16_t, FRAME_COLORS> palette{};
  std::vector<bool> changedRows;
};

bool equalBytes(const uint8_t *left, const uint8_t *right, std::size_t count);

class IndexedRasterizer {
public:
  using Compare = bool (*)(const uint8_t *left, const uint8_t *right,
                           std::size_t count);

  explicit IndexedRasterizer(Compare compare = equalBytes);

  void rasterize(const Display &display, IndexedFrame &frame);

private:
  struct Mapping {
    std::array<uint8_t, FRAME_COLORS> slots{};
    bool mapped = false;
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

  void placeBlock(const Layer &layer, Mapping &mapping);
  Mapping &mapped(const Layer &layer, Mapping &mapping);
  uint8_t slot(uint16_t color);
  void remember(uint16_t color, uint8_t slot);
  uint8_t nearestSlot(uint16_t color) const;
  Drawn drawn(const Layer &layer, Mapping &mapping);
  void place(const Display &display);
  bool isShown(std::size_t index, int row) const;
  bool hasChanged(const Display &display, int row) const;
  void drawRow(const Display &display, int row, uint8_t border);
  uint8_t drawSpan(const Layer &layer, const Placed &placed,
                   const Mapping &shown, int row, uint8_t *saved);
  void recolor(const Layer &layer, int row);

  Compare m_compare;
  IndexedFrame *m_frame = nullptr;
  const IndexedFrame *m_lastFrame = nullptr;
  std::vector<int16_t> m_colorSlots;
  std::vector<uint16_t> m_assignedColors;
  std::array<bool, FRAME_COLORS> m_used{};
  std::size_t m_nextFree = 0;
  std::vector<Mapping> m_mappings;
  std::vector<Drawn> m_drawn;
  std::vector<Drawn> m_lastDrawn;
  int m_lastBorder = -1;
  std::vector<Placed> m_placed;
  std::vector<int> m_topLayers;
  std::vector<std::vector<bool>> m_recolored;
  std::vector<std::vector<uint8_t>> m_saved;
  Mapping m_rowMapping;
};

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_INDEXEDRASTERIZER_H_
