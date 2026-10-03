#ifndef SYSTEMS_GRAPHICS_DISPLAY_H_
#define SYSTEMS_GRAPHICS_DISPLAY_H_

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace graphics {

inline constexpr int CHANNEL_STEP = 17;
inline constexpr int PAL_HERTZ = 50;
inline constexpr int NTSC_HERTZ = 60;

struct RowColor {
  int row = 0;
  uint8_t index = 0;
  uint16_t color = 0;
};

class RowColors {
public:
  RowColors() = default;
  RowColors(std::initializer_list<RowColor> rows);

  bool empty() const;
  std::size_t size() const;
  const RowColor *begin() const;
  const RowColor *end() const;
  RowColor *begin();
  RowColor *end();
  void push_back(const RowColor &row);
  void pop_back();
  void clear();
  bool shares(const RowColors &other) const;

private:
  std::vector<RowColor> &owned();

  std::shared_ptr<std::vector<RowColor>> m_rows;
};

struct Layer {
  const uint8_t *pixels = nullptr;
  int stride = 0;
  int sourceColumns = 0;
  int sourceRows = 0;
  int sourceX = 0;
  int sourceY = 0;
  int sourceStep = 1;
  int repeat = 1;
  bool wrap = false;
  int left = 0;
  int top = 0;
  int columns = 0;
  int rows = 0;
  uint8_t mask = 0xFF;
  std::vector<uint16_t> palette;
  RowColors rowColors;
  uint32_t revision = 0;
};

struct Display {
  int width = 0;
  int height = 0;
  int displayHeight = 0;
  uint16_t border = 0;
  std::vector<Layer> layers;
  uint32_t revision = 0;
};

uint32_t newRevision();
Layer solidLayer(uint16_t color, int top, int rows, int columns);
void cropRows(Display &display, int first, int count);
uint32_t toArgb(uint16_t color);
void rasterize(const Display &display, std::vector<uint32_t> &argb);

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_GRAPHICS_DISPLAY_H_
