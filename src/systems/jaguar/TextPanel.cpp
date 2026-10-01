#include "TextPanel.h"

#include "ConsoleFont.h"

#include <algorithm>

namespace openfranko::src::systems::jaguar {

void drawPanelLine(uint16_t *pixels, int width, int line, const char *text,
                   uint16_t ink, uint16_t paper) {
  uint16_t *top = pixels + line * CELL_HEIGHT * width;
  std::fill(top, top + CELL_HEIGHT * width, paper);
  const int columns = width / CELL_WIDTH;
  for (int column = 0; column < columns && text[column] != '\0'; ++column) {
    const uint8_t *rows = glyph(static_cast<unsigned char>(text[column]));
    if (!rows) {
      continue;
    }
    for (int row = 0; row < GLYPH_HEIGHT; ++row) {
      for (int bit = 0; bit < GLYPH_WIDTH; ++bit) {
        if (rows[row] & (0x10 >> bit)) {
          top[row * width + column * CELL_WIDTH + bit] = ink;
        }
      }
    }
  }
}

} // namespace openfranko::src::systems::jaguar
