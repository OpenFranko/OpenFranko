#include "Font.h"

#include <cstddef>

namespace openfranko::src::engine::street::core {
namespace {

constexpr int LINE_WIDTH = 280;
constexpr int GLYPH_WIDTH = 16;
constexpr int GLYPH_OFFSET = 6;

} // namespace

void font(const std::string &text, int y, const Paste &paste) {
  const int length = static_cast<int>(text.size());
  const int x = (LINE_WIDTH - length * GLYPH_WIDTH) / 2;
  for (int i = 1; i <= length; ++i) {
    const int image =
        static_cast<unsigned char>(text[static_cast<std::size_t>(i - 1)]) +
        GLYPH_OFFSET;
    paste(i * GLYPH_WIDTH + x, y, image);
  }
}

} // namespace openfranko::src::engine::street::core
