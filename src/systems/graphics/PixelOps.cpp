#include "graphics/PixelOps.h"

#include <cstddef>
#include <cstring>

namespace openfranko::src::systems::graphics::pixels {
namespace {

constexpr int WORD_BYTES = 4;
constexpr uint32_t LOW_BITS = 0x01010101u;
constexpr uint32_t HIGH_BITS = 0x80808080u;

bool hasZeroByte(uint32_t word) {
  return ((word - LOW_BITS) & ~word & HIGH_BITS) != 0;
}

void drawRow(const uint8_t *source, uint8_t *target, int width,
             bool transparent, bool mirrored) {
  if (!transparent && !mirrored) {
    std::memmove(target, source, static_cast<std::size_t>(width));
    return;
  }
  int column = 0;
  for (; column + WORD_BYTES <= width; column += WORD_BYTES) {
    uint8_t bytes[WORD_BYTES];
    if (mirrored) {
      const uint8_t *from = source + width - WORD_BYTES - column;
      bytes[0] = from[3];
      bytes[1] = from[2];
      bytes[2] = from[1];
      bytes[3] = from[0];
    } else {
      std::memcpy(bytes, source + column, WORD_BYTES);
    }
    uint32_t word = 0;
    std::memcpy(&word, bytes, WORD_BYTES);
    if (!transparent || !hasZeroByte(word)) {
      std::memcpy(target + column, bytes, WORD_BYTES);
      continue;
    }
    for (int byte = 0; byte < WORD_BYTES; ++byte) {
      if (bytes[byte] != 0) {
        target[column + byte] = bytes[byte];
      }
    }
  }
  for (; column < width; ++column) {
    const uint8_t value = source[mirrored ? width - 1 - column : column];
    if (value != 0 || !transparent) {
      target[column] = value;
    }
  }
}

} // namespace

void copy(Source source, Target target, int width, int height) {
  for (int row = 0; row < height; ++row) {
    std::memmove(
        target.pixels + static_cast<std::ptrdiff_t>(row) * target.pitch,
        source.pixels + static_cast<std::ptrdiff_t>(row) * source.pitch,
        static_cast<std::size_t>(width));
  }
}

void move(Source source, Target target, int width, int height) {
  if (target.pixels <= source.pixels) {
    copy(source, target, width, height);
    return;
  }
  for (int row = height - 1; row >= 0; --row) {
    std::memmove(
        target.pixels + static_cast<std::ptrdiff_t>(row) * target.pitch,
        source.pixels + static_cast<std::ptrdiff_t>(row) * source.pitch,
        static_cast<std::size_t>(width));
  }
}

void draw(Source source, Target target, int width, int height, bool transparent,
          bool mirrored) {
  for (int row = 0; row < height; ++row) {
    drawRow(source.pixels + static_cast<std::ptrdiff_t>(row) * source.pitch,
            target.pixels + static_cast<std::ptrdiff_t>(row) * target.pitch,
            width, transparent, mirrored);
  }
}

void fill(Target target, int width, int height, uint8_t value) {
  if (width == target.pitch) {
    std::memset(target.pixels, value,
                static_cast<std::size_t>(width) *
                    static_cast<std::size_t>(height));
    return;
  }
  for (int row = 0; row < height; ++row) {
    std::memset(target.pixels + static_cast<std::ptrdiff_t>(row) * target.pitch,
                value, static_cast<std::size_t>(width));
  }
}

} // namespace openfranko::src::systems::graphics::pixels
