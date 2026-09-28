#include "SystemText.h"

#include <array>
#include <cstddef>

namespace openfranko::src::engine::street {
namespace {

struct Glyph {
  char character;
  std::array<uint8_t, SYSTEM_FONT_HEIGHT> rows;
};

constexpr std::array<Glyph, 11> GLYPHS = {{
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {':', {0x00, 0x30, 0x30, 0x00, 0x30, 0x30, 0x00, 0x00}},
    {'C', {0x7C, 0xC6, 0xC0, 0xC0, 0xC0, 0xC6, 0x7C, 0x00}},
    {'D', {0xF8, 0xCC, 0xC6, 0xC6, 0xC6, 0xCC, 0xF8, 0x00}},
    {'E', {0xFE, 0xC0, 0xC0, 0xFC, 0xC0, 0xC0, 0xFE, 0x00}},
    {'K', {0xC6, 0xCC, 0xD8, 0xF0, 0xD8, 0xCC, 0xC6, 0x00}},
    {'N', {0xC6, 0xE6, 0xE6, 0xD6, 0xCE, 0xCE, 0xC6, 0x00}},
    {'O', {0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00}},
    {'R', {0xFC, 0xC6, 0xC6, 0xFC, 0xD8, 0xCC, 0xC6, 0x00}},
    {'T', {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}},
    {'Z', {0xFE, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFE, 0x00}},
}};

const Glyph *find(char character) {
  for (const Glyph &glyph : GLYPHS) {
    if (glyph.character == character) {
      return &glyph;
    }
  }
  return nullptr;
}

} // namespace

void drawSystemText(IndexedSurface &surface, int x, int baseline,
                    const std::string &text, uint8_t ink, uint8_t paper) {
  const int top = baseline - SYSTEM_FONT_BASELINE;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const Glyph *glyph = find(text[i]);
    const int left = x + static_cast<int>(i) * SYSTEM_FONT_WIDTH;
    for (int row = 0; row < SYSTEM_FONT_HEIGHT; ++row) {
      const uint8_t bits =
          glyph ? glyph->rows[static_cast<std::size_t>(row)] : 0;
      for (int column = 0; column < SYSTEM_FONT_WIDTH; ++column) {
        const uint8_t color = (bits & (0x80 >> column)) ? ink : paper;
        surface.clear(color, left + column, top + row, left + column + 1,
                      top + row + 1);
      }
    }
  }
}

} // namespace openfranko::src::engine::street
