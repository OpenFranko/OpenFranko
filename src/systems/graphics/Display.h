#ifndef SYSTEMS_DISPLAY_H_
#define SYSTEMS_DISPLAY_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {

struct RowColor {
  int row = 0;
  uint8_t index = 0;
  uint16_t color = 0;
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
  std::vector<RowColor> rowColors;
};

struct Display {
  int width = 0;
  int height = 0;
  int displayHeight = 0;
  uint16_t border = 0;
  std::vector<Layer> layers;
};

Layer solidLayer(uint16_t color, int top, int rows, int columns);
void cropRows(Display &display, int first, int count);
uint32_t toArgb(uint16_t color);
void rasterize(const Display &display, std::vector<uint32_t> &argb);

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_DISPLAY_H_
