#include "graphics/PixelOps.h"

#include "jaguar/Blitter.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace openfranko::src::systems::graphics::pixels {
namespace {

namespace blitter = systems::jaguar::blitter;

constexpr int SMALL_AREA = 48;
constexpr int SCRATCH_BYTES = 16384;
constexpr int PHRASE = 8;

alignas(PHRASE) uint8_t scratch[SCRATCH_BYTES];

int product(int16_t left, int16_t right) {
  int32_t result = left;
  asm("muls.w %1,%0" : "+d"(result) : "d"(right) : "cc");
  return result;
}

bool isSmall(int width, int height) {
  return !blitter::isQueued() &&
         (width < PHRASE || product(static_cast<int16_t>(width),
                                    static_cast<int16_t>(height)) < SMALL_AREA);
}

blitter::Source from(Source source) { return {source.pixels, source.pitch}; }

blitter::Area to(Target target) { return {target.pixels, target.pitch}; }

void drawLoop(Source source, Target target, int width, int height,
              bool transparent, bool mirrored) {
  for (int row = 0; row < height; ++row) {
    for (int column = 0; column < width; ++column) {
      const uint8_t value =
          source.pixels[mirrored ? width - 1 - column : column];
      if (value != 0 || !transparent) {
        target.pixels[column] = value;
      }
    }
    source.pixels += source.pitch;
    target.pixels += target.pitch;
  }
}

bool overlaps(Source source, Target target, int width, int height) {
  const uint8_t *sourceFirst = source.pixels;
  const uint8_t *sourceLast = source.pixels +
                              product(static_cast<int16_t>(height - 1),
                                      static_cast<int16_t>(source.pitch)) +
                              width;
  const uint8_t *targetFirst = target.pixels;
  const uint8_t *targetLast = target.pixels +
                              product(static_cast<int16_t>(height - 1),
                                      static_cast<int16_t>(target.pitch)) +
                              width;
  return targetFirst < sourceLast && sourceFirst < targetLast;
}

void bounce(Source source, Target target, int width, int height) {
  const int pitch = (width + PHRASE - 1) & ~(PHRASE - 1);
  const int band = std::max(1, SCRATCH_BYTES / pitch);
  for (int last = height; last > 0;) {
    const int rows = std::min(band, last);
    const int first = last - rows;
    const Source bandSource{source.pixels +
                                product(static_cast<int16_t>(first),
                                        static_cast<int16_t>(source.pitch)),
                            source.pitch};
    const Target bandTarget{target.pixels +
                                product(static_cast<int16_t>(first),
                                        static_cast<int16_t>(target.pitch)),
                            target.pitch};
    blitter::copy(from(bandSource), {scratch, pitch}, width, rows);
    blitter::copy({scratch, pitch}, to(bandTarget), width, rows);
    last = first;
  }
}

} // namespace

void copy(Source source, Target target, int width, int height) {
  if (width <= 0 || height <= 0) {
    return;
  }
  if (isSmall(width, height)) {
    blitter::wait();
    for (int row = 0; row < height; ++row) {
      std::memmove(target.pixels, source.pixels,
                   static_cast<std::size_t>(width));
      source.pixels += source.pitch;
      target.pixels += target.pitch;
    }
    return;
  }
  blitter::copy(from(source), to(target), width, height);
}

void move(Source source, Target target, int width, int height) {
  if (width <= 0 || height <= 0) {
    return;
  }
  if (!overlaps(source, target, width, height)) {
    copy(source, target, width, height);
    return;
  }
  if (target.pixels < source.pixels) {
    if (isSmall(width, height)) {
      copy(source, target, width, height);
    } else {
      blitter::copy(from(source), to(target), width, height);
    }
    return;
  }
  if (isSmall(width, height) || width > SCRATCH_BYTES) {
    blitter::wait();
    for (int row = height - 1; row >= 0; --row) {
      std::memmove(target.pixels + product(static_cast<int16_t>(row),
                                           static_cast<int16_t>(target.pitch)),
                   source.pixels + product(static_cast<int16_t>(row),
                                           static_cast<int16_t>(source.pitch)),
                   static_cast<std::size_t>(width));
    }
    return;
  }
  bounce(source, target, width, height);
}

void draw(Source source, Target target, int width, int height, bool transparent,
          bool mirrored) {
  if (width <= 0 || height <= 0) {
    return;
  }
  if (isSmall(width, height)) {
    blitter::wait();
    drawLoop(source, target, width, height, transparent, mirrored);
    return;
  }
  if (mirrored) {
    blitter::copyMirrored(from(source), to(target), width, height, transparent);
  } else if (transparent) {
    blitter::copyMasked(from(source), to(target), width, height);
  } else {
    blitter::copy(from(source), to(target), width, height);
  }
}

void fill(Target target, int width, int height, uint8_t value) {
  if (width <= 0 || height <= 0) {
    return;
  }
  if (isSmall(width, height)) {
    blitter::wait();
    for (int row = 0; row < height; ++row) {
      std::memset(target.pixels, value, static_cast<std::size_t>(width));
      target.pixels += target.pitch;
    }
    return;
  }
  blitter::fill(to(target), width, height, value);
}

void finish() { blitter::wait(); }

} // namespace openfranko::src::systems::graphics::pixels
