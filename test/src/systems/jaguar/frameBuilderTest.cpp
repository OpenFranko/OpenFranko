#include "../../../../src/systems/jaguar/FrameBuilder.h"
#include "../../../../src/systems/graphics/Display.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace openfranko::src::systems;
using namespace openfranko::src::systems::jaguar;

namespace {

constexpr uint32_t ADDRESS_MASK = 0xFFFFFF;
constexpr uint32_t LINK_MASK = 0x3FFFFF;
constexpr std::size_t ARENA_BYTES = 4 << 20;
constexpr int LIVE_PHRASES = 96;

Geometry palGeometry() {
  Geometry geometry;
  geometry.columns = 345;
  geometry.rows = 287;
  geometry.firstHalfLine = 35;
  geometry.lastHalfLine = 609;
  return geometry;
}

Geometry ntscGeometry() {
  Geometry geometry;
  geometry.ntsc = true;
  geometry.columns = 352;
  geometry.rows = 241;
  geometry.firstHalfLine = 25;
  geometry.lastHalfLine = 507;
  return geometry;
}

uint64_t bits(uint64_t value, int shift, int count) {
  return (value >> shift) & ((uint64_t(1) << count) - 1);
}

class Arena {
public:
  Arena() : m_memory(ARENA_BYTES + 64, 0) {}

  uint8_t *allocate(std::size_t size, std::size_t phase = 0) {
    std::size_t at = (m_used + 31) / 32 * 32 + phase;
    m_used = at + size;
    REQUIRE(m_used <= ARENA_BYTES);
    return base() + at;
  }

  uint32_t address(const void *pointer) const {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pointer));
  }

  const uint8_t *at(uint32_t address) const {
    const uint32_t start = this->address(base()) & ADDRESS_MASK;
    return base() + ((address - start) & ADDRESS_MASK);
  }

private:
  uint8_t *base() const {
    const uintptr_t raw = reinterpret_cast<uintptr_t>(m_memory.data());
    return const_cast<uint8_t *>(m_memory.data()) + ((32 - raw % 32) % 32);
  }

  std::vector<uint8_t> m_memory;
  std::size_t m_used = 0;
};

using Screen = std::vector<std::vector<uint16_t>>;

void drawBitmap(const Arena &arena, const std::vector<uint64_t> &list,
                std::size_t at, bool scaled, std::vector<uint16_t> &line,
                const std::array<uint16_t, 256> &clut) {
  const uint64_t header = list[at];
  const uint64_t layout = list[at + 1];
  const uint8_t *data =
      arena.at(static_cast<uint32_t>(bits(header, 43, 21) << 3));
  const int x = static_cast<int>(bits(layout, 0, 12) ^ 0x800) - 0x800;
  const int depth = static_cast<int>(bits(layout, 12, 3));
  const int pitch = static_cast<int>(bits(layout, 15, 3));
  const int phrases = static_cast<int>(bits(layout, 28, 10));
  const int firstPixel = static_cast<int>(bits(layout, 49, 6));
  const int scale = scaled ? static_cast<int>(bits(list[at + 2], 0, 8)) : 32;
  const int pixelBits = 1 << depth;
  const int perPhrase = 64 / pixelBits;
  int column = x;
  int accumulated = 0;
  for (int index = firstPixel / pixelBits; index < phrases * perPhrase;
       ++index) {
    const uint8_t *phrase = data + (index / perPhrase) * pitch * 8;
    const int within = index % perPhrase;
    uint16_t color = 0;
    if (pixelBits == 8) {
      color = clut[phrase[within]];
    } else if (pixelBits == 16) {
      color = static_cast<uint16_t>(phrase[within * 2] << 8 |
                                    phrase[within * 2 + 1]);
    }
    accumulated += scale;
    while (accumulated >= 32) {
      if (column >= 0 && column < static_cast<int>(line.size())) {
        line[static_cast<std::size_t>(column)] = color;
      }
      ++column;
      accumulated -= 32;
    }
  }
}

Screen simulate(const Arena &arena, const BuiltFrame &frame,
                uint32_t liveAddress, const Geometry &geometry) {
  std::vector<uint64_t> list = frame.phrases;
  std::array<uint16_t, 256> clut = frame.clut;
  std::size_t copper = 0;
  Screen screen(
      static_cast<std::size_t>(geometry.rows),
      std::vector<uint16_t>(static_cast<std::size_t>(geometry.columns)));
  for (int halfLine = geometry.firstHalfLine; halfLine < geometry.lastHalfLine;
       halfLine += 2) {
    std::vector<uint16_t> &line = screen[static_cast<std::size_t>(
        (halfLine - geometry.firstHalfLine) / 2)];
    std::fill(line.begin(), line.end(), frame.background);
    std::size_t at = 0;
    for (int guard = 0; guard < 1000; ++guard) {
      const uint64_t phrase = list[at];
      const int type = static_cast<int>(bits(phrase, 0, 3));
      const int ypos = static_cast<int>(bits(phrase, 3, 11));
      const std::size_t link =
          (((static_cast<uint32_t>(bits(phrase, 24, 19)) << 3) - liveAddress) &
           LINK_MASK) /
          8;
      if (type == 4) {
        break;
      }
      if (type == 3) {
        const int condition = static_cast<int>(bits(phrase, 14, 3));
        const bool taken = (condition == 0 && ypos == halfLine) ||
                           (condition == 1 && ypos > halfLine) ||
                           (condition == 2 && ypos < halfLine);
        at = taken ? link : at + 1;
        continue;
      }
      if (type == 2) {
        while (copper < frame.copper.size() &&
               static_cast<int>(frame.copper[copper] >> 16) <= halfLine) {
          const uint32_t count = frame.copper[copper] & 0xFFFF;
          for (uint32_t entry = 1; entry <= count; ++entry) {
            const uint32_t value = frame.copper[copper + entry];
            clut[(value >> 16) / 2] = static_cast<uint16_t>(value);
          }
          copper += 1 + count;
        }
        ++at;
        continue;
      }
      const bool scaled = type == 1;
      int height = static_cast<int>(bits(phrase, 14, 10));
      if (halfLine >= ypos && height > 0) {
        drawBitmap(arena, list, at, scaled, line, clut);
        uint32_t data = static_cast<uint32_t>(bits(phrase, 43, 21));
        const uint32_t dataWidth =
            static_cast<uint32_t>(bits(list[at + 1], 18, 10));
        if (scaled) {
          const int verticalScale = static_cast<int>(bits(list[at + 2], 8, 8));
          int remainder = static_cast<int>(bits(list[at + 2], 16, 8)) - 32;
          while (remainder <= 0 && height > 0) {
            remainder += verticalScale;
            --height;
            data += dataWidth;
          }
          list[at + 2] = (list[at + 2] & ~(uint64_t(0xFF) << 16)) |
                         uint64_t(remainder & 0xFF) << 16;
        } else {
          data += dataWidth;
          --height;
        }
        list[at] = (list[at] &
                    ~((uint64_t(0x3FF) << 14) | (uint64_t(0x1FFFFF) << 43))) |
                   uint64_t(height & 0x3FF) << 14 |
                   uint64_t(data & 0x1FFFFF) << 43;
      }
      at = link;
    }
  }
  return screen;
}

uint16_t amiga(uint16_t rgb16) {
  return static_cast<uint16_t>(((rgb16 >> 12) & 15) << 8 |
                               ((rgb16 & 0x3F) >> 2) << 4 |
                               ((rgb16 >> 7) & 15));
}

uint16_t amiga(uint32_t argb) {
  return static_cast<uint16_t>(((argb >> 16) & 0xFF) / 17 << 8 |
                               ((argb >> 8) & 0xFF) / 17 << 4 |
                               (argb & 0xFF) / 17);
}

int mismatches(const graphics::Display &original, const Geometry &geometry) {
  Arena arena;
  graphics::Display display = original;
  for (graphics::Layer &layer : display.layers) {
    if (!layer.pixels) {
      continue;
    }
    const std::size_t size = static_cast<std::size_t>(layer.stride) *
                             static_cast<std::size_t>(layer.sourceRows);
    uint8_t *copy =
        arena.allocate(size, reinterpret_cast<uintptr_t>(layer.pixels) % 8);
    std::memcpy(copy, layer.pixels, size);
    layer.pixels = copy;
  }
  uint8_t *solid = arena.allocate(8);
  uint8_t *live = arena.allocate(LIVE_PHRASES * 8);
  BuiltFrame frame;
  buildFrame(display, geometry, arena.address(live), arena.address(solid),
             nullptr, 0, frame);
  REQUIRE(frame.phrases.size() <= static_cast<std::size_t>(LIVE_PHRASES));
  const Screen screen = simulate(arena, frame, arena.address(live), geometry);
  std::vector<uint32_t> argb;
  graphics::rasterize(display, argb);
  const Placement placement = placeDisplay(display, geometry);
  int wrong = 0;
  for (int row = 0; row < display.height; ++row) {
    const int screenRow = row + placement.top;
    if (screenRow < 0 || screenRow >= geometry.rows) {
      continue;
    }
    for (int column = 0; column < display.width;
         column += placement.halfWidth) {
      const int screenColumn = placement.left + column / placement.halfWidth;
      if (screenColumn < 0 || screenColumn >= geometry.columns) {
        continue;
      }
      const uint16_t got =
          amiga(screen[static_cast<std::size_t>(screenRow)]
                      [static_cast<std::size_t>(screenColumn)]);
      bool found = false;
      for (int pixel = 0; pixel < placement.halfWidth; ++pixel) {
        found = found || amiga(argb[static_cast<std::size_t>(
                             row * display.width + column + pixel)]) == got;
      }
      if (!found) {
        ++wrong;
      }
    }
  }
  return wrong;
}

graphics::Layer layer(int width, int height, int colors, uint8_t seed) {
  static std::vector<std::vector<uint8_t>> storage;
  storage.emplace_back(static_cast<std::size_t>(width * height));
  std::vector<uint8_t> &pixels = storage.back();
  for (int at = 0; at < width * height; ++at) {
    pixels[static_cast<std::size_t>(at)] =
        static_cast<uint8_t>((at * 7 + at / width * 3 + seed) % colors);
  }
  graphics::Layer result;
  result.pixels = pixels.data();
  result.stride = width;
  result.sourceColumns = width;
  result.sourceRows = height;
  result.columns = width;
  result.rows = height;
  for (int color = 0; color < colors; ++color) {
    result.palette.push_back(
        static_cast<uint16_t>((color * 0x137 + seed * 0x51) & 0xFFF));
  }
  return result;
}

} // namespace

SCENARIO("Jaguar frames show what the desktop rasterizer draws") {
  for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
    GIVEN("A full screen picture with one palette") {
      graphics::Display display;
      display.width = 320;
      display.height = 256;
      display.border = 0x123;
      display.layers.push_back(layer(320, 256, 32, 1));
      THEN("Every pixel matches") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("An overscan picture wider than the screen") {
      graphics::Display display;
      display.width = 368;
      display.height = 280;
      display.layers.push_back(layer(368, 280, 32, 8));
      THEN("The columns that fit start at the left edge") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A play screen over a panel with its own palette") {
      graphics::Display display;
      display.width = 304;
      display.height = 255;
      display.border = 0x555;
      display.layers.push_back(layer(320, 222, 16, 2));
      display.layers.back().columns = 304;
      graphics::Layer panel = layer(304, 32, 8, 3);
      panel.top = 223;
      display.layers.push_back(panel);
      THEN("The copper switches palettes between them") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A picture with colours changed on some rows") {
      graphics::Display display;
      display.width = 320;
      display.height = 200;
      graphics::Layer picture = layer(320, 200, 16, 4);
      for (int row = 10; row < 190; row += 7) {
        picture.rowColors.push_back(
            {row, static_cast<uint8_t>(row % 16),
             static_cast<uint16_t>(row * 0x21 & 0xFFF)});
      }
      display.layers.push_back(picture);
      THEN("The changes land on their rows") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("Layers scrolled to columns between phrases") {
      graphics::Display display;
      display.width = 320;
      display.height = 240;
      graphics::Layer scrolled = layer(336, 100, 16, 6);
      scrolled.sourceX = 3;
      scrolled.columns = 320;
      display.layers.push_back(scrolled);
      graphics::Layer doubled = layer(336, 60, 32, 7);
      doubled.sourceX = 5;
      doubled.top = 110;
      doubled.columns = 320;
      doubled.rows = 120;
      doubled.repeat = 2;
      display.layers.push_back(doubled);
      THEN("The first pixel field skips the columns before them") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A layer one column past a phrase that ends one past the next") {
      graphics::Display display;
      display.width = 320;
      display.height = 100;
      graphics::Layer scrolled = layer(336, 100, 16, 9);
      scrolled.sourceX = 1;
      scrolled.columns = 320;
      display.layers.push_back(scrolled);
      THEN("Its last column is still shown") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A doubled hires layer over a solid one sharing its colour 0") {
      graphics::Display display;
      display.width = 640;
      display.height = 220;
      display.layers.push_back(graphics::solidLayer(0xA50, 0, 220, 640));
      graphics::Layer doubled = layer(656, 120, 64, 5);
      doubled.sourceX = 8;
      doubled.sourceY = 5;
      doubled.left = 16;
      doubled.top = 7;
      doubled.columns = 600;
      doubled.rows = 200;
      doubled.repeat = 2;
      doubled.palette[0] = 0xA50;
      display.layers.push_back(doubled);
      THEN("Every visible pixel matches at half width") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }
  }
}
