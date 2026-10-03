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
constexpr int LINE_PHRASES = 300;

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
  const bool transparent = bits(layout, 47, 1) != 0;
  const int pixelBits = 1 << depth;
  const int perPhrase = 64 / pixelBits;
  int column = x;
  int accumulated = 0;
  for (int index = firstPixel / pixelBits; index < phrases * perPhrase;
       ++index) {
    const uint8_t *phrase = data + (index / perPhrase) * pitch * 8;
    const int within = index % perPhrase;
    uint16_t color = 0;
    bool clear = false;
    if (pixelBits == 8) {
      color = clut[phrase[within]];
      clear = phrase[within] == 0;
    } else if (pixelBits == 16) {
      color = static_cast<uint16_t>(phrase[within * 2] << 8 |
                                    phrase[within * 2 + 1]);
      clear = color == 0;
    }
    accumulated += scale;
    while (accumulated >= 32) {
      if (column >= 0 && column < static_cast<int>(line.size()) &&
          !(transparent && clear)) {
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
          for (uint32_t entry = 0; entry < count; ++entry) {
            const uint32_t offset = frame.copper[copper + 1 + 2 * entry];
            const uint32_t colors = frame.copper[copper + 2 + 2 * entry];
            clut[offset / 2] = static_cast<uint16_t>(colors >> 16);
            clut[offset / 2 + 1] = static_cast<uint16_t>(colors);
          }
          copper += 1 + 2 * count;
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

class ArenaBuffers : public TranslationBuffers {
public:
  explicit ArenaBuffers(Arena &arena) : m_arena(arena) {}

  uint8_t *buffer(const uint8_t *, std::size_t bytes) override {
    return m_arena.allocate(bytes);
  }

private:
  Arena &m_arena;
};

FrameMemory frameMemory(Arena &arena, ArenaBuffers &buffers) {
  uint8_t *solid = arena.allocate(SOLID_PHRASES * 8);
  for (int value = 0; value < SOLID_PHRASES; ++value) {
    std::memset(solid + value * 8, value, 8);
  }
  uint8_t *live = arena.allocate(LIVE_PHRASES * 8);
  uint8_t *lines = arena.allocate(LINE_PHRASES * 8, 4);
  return {arena.address(live), arena.address(solid), &buffers, lines,
          LINE_PHRASES};
}

BuiltFrame build(Arena &arena, const graphics::Display &display,
                 const Geometry &geometry, FrameMemory &memory) {
  ArenaBuffers buffers(arena);
  memory = frameMemory(arena, buffers);
  BuiltFrame frame;
  buildFrame(display, geometry, memory, nullptr, 0, frame);
  for (const Translation &translation : frame.translations) {
    translateOnCpu(translation);
  }
  memory.buffers = nullptr;
  return frame;
}

graphics::Display inArena(Arena &arena, const graphics::Display &original) {
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
  return display;
}

int wrongPixels(const Arena &arena, const BuiltFrame &frame,
                const FrameMemory &memory, const graphics::Display &display,
                const Geometry &geometry) {
  const Screen screen = simulate(arena, frame, memory.liveAddress, geometry);
  std::vector<uint32_t> argb;
  graphics::rasterize(display, argb);
  const Placement placement = placeDisplay(display, geometry);
  int wrong = 0;
  for (int row = 0; row < display.height; row += placement.rowsPerLine) {
    const int screenRow = row / placement.rowsPerLine + placement.top;
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

int mismatches(const graphics::Display &original, const Geometry &geometry) {
  Arena arena;
  const graphics::Display display = inArena(arena, original);
  FrameMemory memory;
  const BuiltFrame frame = build(arena, display, geometry, memory);
  REQUIRE(frame.phrases.size() <= static_cast<std::size_t>(LIVE_PHRASES));
  return wrongPixels(arena, frame, memory, display, geometry);
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

    GIVEN("A scrolling picture with a colour changed on every row") {
      graphics::Display display;
      display.width = 320;
      display.height = 256;
      graphics::Layer picture = layer(320, 256, 32, 9);
      picture.wrap = true;
      for (int row = 28; row < 256; ++row) {
        picture.rowColors.push_back(
            {row, 0, static_cast<uint16_t>(row * 0x123 & 0xFFF)});
      }
      display.layers.push_back(picture);
      THEN("Each frame matches as it scrolls and its colours change") {
        REQUIRE(mismatches(display, geometry) == 0);
        display.layers.back().sourceX = 5;
        REQUIRE(mismatches(display, geometry) == 0);
        for (graphics::RowColor &change : display.layers.back().rowColors) {
          change.color = static_cast<uint16_t>((change.color + 0x111) & 0xFFF);
        }
        REQUIRE(mismatches(display, geometry) == 0);
        display.layers.back().palette[0] = 0x0F0;
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A masked layer whose rows change colours shared with the next") {
      graphics::Display display;
      display.width = 320;
      display.height = 120;
      graphics::Layer picture = layer(320, 120, 4, 5);
      picture.mask = 0x03;
      for (int row = 4; row < 120; ++row) {
        picture.rowColors.push_back(
            {row, static_cast<uint8_t>(row % 4),
             static_cast<uint16_t>(row * 0x35 & 0xFFF)});
        picture.rowColors.push_back(
            {row, static_cast<uint8_t>((row + 1) % 4),
             static_cast<uint16_t>(row * 0x17 & 0xFFF)});
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

    GIVEN("A laced play screen over a doubled panel, as in 320x512 mode") {
      graphics::Display display;
      display.width = 304;
      display.height = 510;
      display.displayHeight = 255;
      display.border = 0x555;
      graphics::Layer play = layer(320, 444, 16, 6);
      play.sourceY = 6;
      play.columns = 304;
      play.rows = 510;
      display.layers.push_back(play);
      graphics::Layer panel = layer(304, 32, 8, 7);
      panel.top = 344;
      panel.repeat = 2;
      panel.rows = 64;
      display.layers.push_back(panel);
      display.layers.push_back(graphics::solidLayer(0x000, 0, 30, 304));
      const Placement placement = placeDisplay(display, geometry);

      THEN("Each line shows every other row, so the play screen halves "
           "and the panel keeps its 32 lines") {
        REQUIRE(placement.rowsPerLine == 2);
        REQUIRE(placement.top == (geometry.rows - 255) / 2);
        const LayerArea area =
            visibleArea(display, display.layers[1], placement, geometry);
        REQUIRE((area.lastRow - area.firstRow) / placement.rowsPerLine == 32);
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

SCENARIO("Every frame starts with the same header, so the list can be swapped "
         "while the Object Processor reads it in the blank") {
  GIVEN("A plain picture, a play screen over a panel and a hires screen") {
    graphics::Display plain;
    plain.width = 320;
    plain.height = 256;
    plain.displayHeight = 256;
    plain.layers.push_back(layer(320, 256, 32, 1));

    graphics::Display stage;
    stage.width = 304;
    stage.height = 255;
    stage.displayHeight = 255;
    stage.layers.push_back(layer(320, 222, 16, 2));
    stage.layers.back().columns = 304;
    graphics::Layer panel = layer(304, 32, 8, 3);
    panel.top = 223;
    stage.layers.push_back(panel);

    graphics::Display hires;
    hires.width = 640;
    hires.height = 200;
    hires.displayHeight = 400;
    hires.layers.push_back(layer(640, 200, 16, 4));

    const std::vector<const graphics::Display *> displays{&plain, &stage,
                                                          &hires};

    THEN("The first four phrases are identical for every display") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        ArenaBuffers buffers(arena);
        const FrameMemory memory = frameMemory(arena, buffers);
        std::vector<std::vector<uint64_t>> headers;
        for (const graphics::Display *display : displays) {
          BuiltFrame frame;
          buildFrame(*display, geometry, memory, nullptr, 0, frame);
          REQUIRE(frame.phrases.size() > 4);
          headers.emplace_back(frame.phrases.begin(),
                               frame.phrases.begin() + 4);
        }
        REQUIRE(headers[1] == headers[0]);
        REQUIRE(headers[2] == headers[0]);
      }
    }

    THEN("Every display still matches the desktop rasterizer") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        for (const graphics::Display *display : displays) {
          REQUIRE(mismatches(*display, geometry) == 0);
        }
      }
    }
  }
}

SCENARIO("Layers with clashing palettes get their own colour banks, so the "
         "frame needs no copper") {
  GIVEN("A play screen over a panel with a different palette") {
    graphics::Display stage;
    stage.width = 304;
    stage.height = 255;
    stage.displayHeight = 255;
    stage.layers.push_back(layer(320, 222, 16, 2));
    stage.layers.back().columns = 304;
    graphics::Layer panel = layer(304, 32, 8, 3);
    panel.top = 223;
    stage.layers.push_back(panel);

    THEN("The panel is translated into a bank and the colours still match") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        FrameMemory memory;
        const BuiltFrame frame = build(arena, stage, geometry, memory);
        REQUIRE(frame.copper == std::vector<uint32_t>{COPPER_END});
        REQUIRE(frame.translations.size() == 1);
        REQUIRE(frame.translations[0].flip == 0x80808080u);
        REQUIRE(frame.translations[0].keep == 0xFFFFFFFFu);
        REQUIRE(mismatches(stage, geometry) == 0);
      }
    }
  }

  GIVEN("A black band over the play screen, as in NTSC mode") {
    graphics::Display stage;
    stage.width = 304;
    stage.height = 255;
    stage.displayHeight = 255;
    stage.layers.push_back(layer(320, 255, 16, 5));
    stage.layers.back().columns = 304;
    stage.layers.push_back(graphics::solidLayer(0x000, 0, 19, 304));

    THEN("The band uses a spare colour and needs no copper") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        FrameMemory memory;
        const BuiltFrame frame = build(arena, stage, geometry, memory);
        REQUIRE(frame.copper == std::vector<uint32_t>{COPPER_END});
        REQUIRE(frame.translations.empty());
        REQUIRE(mismatches(stage, geometry) == 0);
      }
    }
  }

  GIVEN("A laced play screen that starts lower, over a doubled panel") {
    graphics::Display stage;
    stage.width = 304;
    stage.height = 510;
    stage.displayHeight = 255;
    graphics::Layer play = layer(320, 444, 16, 6);
    play.sourceY = -120;
    play.columns = 304;
    play.rows = 510;
    stage.layers.push_back(play);
    graphics::Layer panel = layer(304, 32, 8, 7);
    panel.top = 344;
    panel.repeat = 2;
    panel.rows = 64;
    stage.layers.push_back(panel);

    THEN("The empty rows above keep the palette and no copper is needed") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        FrameMemory memory;
        const BuiltFrame frame = build(arena, stage, geometry, memory);
        REQUIRE(frame.copper == std::vector<uint32_t>{COPPER_END});
        REQUIRE(mismatches(stage, geometry) == 0);
      }
    }
  }
}

SCENARIO("Masked layers and row colours on colour 0 need no copper") {
  GIVEN("A masked stage over the panel, as when the ending starts") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer stage = layer(320, 256, 16, 6);
    stage.mask = 0x0F;
    display.layers.push_back(stage);
    graphics::Layer panel = layer(304, 32, 8, 7);
    panel.top = 220;
    display.layers.push_back(panel);

    THEN("The panel is translated into a bank and the colours match") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        FrameMemory memory;
        const BuiltFrame frame = build(arena, display, geometry, memory);
        REQUIRE(frame.copper == std::vector<uint32_t>{COPPER_END});
        REQUIRE(frame.translations.size() == 1);
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }
  }

  GIVEN("Two masked screens with their own palettes, as in the credits") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer lower = layer(320, 164, 16, 8);
    lower.top = 81;
    lower.mask = 0x0F;
    display.layers.push_back(lower);
    graphics::Layer upper = layer(320, 80, 32, 9);
    upper.mask = 0x0F;
    upper.palette.resize(16);
    display.layers.push_back(upper);

    THEN("The smaller screen is masked while it is copied into its bank") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        FrameMemory memory;
        const BuiltFrame frame = build(arena, display, geometry, memory);
        REQUIRE(frame.copper == std::vector<uint32_t>{COPPER_END});
        REQUIRE(frame.translations.size() == 1);
        REQUIRE(frame.translations[0].keep == 0x0F0F0F0Fu);
        REQUIRE(frame.translations[0].flip == 0x80808080u);
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }
  }

  GIVEN("A scrolling picture over a rainbow on colour 0, as on the game over "
        "screen") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer picture = layer(640, 256, 32, 10);
    picture.wrap = true;
    picture.columns = 320;
    picture.sourceX = 37;
    for (int row = 0; row < 240; ++row) {
      picture.rowColors.push_back(
          {row, 0, static_cast<uint16_t>((row / 8 * 0x101) & 0xFFF)});
    }
    display.layers.push_back(picture);

    THEN("A 16-bit object behind the picture shows the row colours") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        FrameMemory memory;
        const BuiltFrame frame = build(arena, display, geometry, memory);
        REQUIRE(frame.copper == std::vector<uint32_t>{COPPER_END});
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }
  }

  GIVEN("A rainbow on a laced display") {
    graphics::Display display;
    display.width = 320;
    display.height = 512;
    display.displayHeight = 256;
    graphics::Layer picture = layer(320, 512, 32, 11);
    for (int row = 0; row < 480; ++row) {
      picture.rowColors.push_back(
          {row, 0, static_cast<uint16_t>((row * 0x31) & 0xFFF)});
    }
    display.layers.push_back(picture);

    THEN("Every shown line takes the colour of its row") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        FrameMemory memory;
        const BuiltFrame frame = build(arena, display, geometry, memory);
        REQUIRE(frame.copper == std::vector<uint32_t>{COPPER_END});
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }
  }
}

SCENARIO("Frames rebuilt into the same slot follow every change") {
  GIVEN("A scrolling rainbow whose colours change between builds") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer picture = layer(640, 256, 32, 12);
    picture.wrap = true;
    picture.columns = 320;
    for (int row = 10; row < 230; ++row) {
      picture.rowColors.push_back(
          {row, 0, static_cast<uint16_t>((row * 0x17) & 0xFFF)});
    }
    display.layers.push_back(picture);

    THEN("Each build shows the colours it was given") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      graphics::Display shown = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      REQUIRE(wrongPixels(arena, frame, memory, shown, geometry) == 0);
      shown.layers.back().sourceX += 3;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      REQUIRE(wrongPixels(arena, frame, memory, shown, geometry) == 0);
      for (graphics::RowColor &change : shown.layers.back().rowColors) {
        change.color = static_cast<uint16_t>((change.color + 0x123) & 0xFFF);
      }
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      REQUIRE(wrongPixels(arena, frame, memory, shown, geometry) == 0);
      shown.layers.back().palette[0] = 0x0F0;
      shown.layers.back().rowColors.pop_back();
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      REQUIRE(wrongPixels(arena, frame, memory, shown, geometry) == 0);
    }
  }

  GIVEN("A rainbow whose rows are shared by copies of the display") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer picture = layer(640, 256, 32, 15);
    picture.wrap = true;
    picture.columns = 320;
    for (int row = 0; row < 240; ++row) {
      picture.rowColors.push_back(
          {row, 0, static_cast<uint16_t>((row * 0x29) & 0xFFF)});
    }
    display.layers.push_back(picture);

    THEN("A shared copy keeps its colours and a changed copy gets new ones") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display shown = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      REQUIRE(wrongPixels(arena, frame, memory, shown, geometry) == 0);
      graphics::Display copy = shown;
      REQUIRE(
          copy.layers.back().rowColors.shares(shown.layers.back().rowColors));
      REQUIRE(sameLayout(shown, copy));
      copy.layers.back().sourceX += 5;
      buildFrame(copy, geometry, memory, nullptr, 0, frame);
      REQUIRE(wrongPixels(arena, frame, memory, copy, geometry) == 0);
      for (graphics::RowColor &change : copy.layers.back().rowColors) {
        change.color = static_cast<uint16_t>((change.color + 0x321) & 0xFFF);
      }
      REQUIRE_FALSE(
          copy.layers.back().rowColors.shares(shown.layers.back().rowColors));
      buildFrame(copy, geometry, memory, nullptr, 0, frame);
      REQUIRE(wrongPixels(arena, frame, memory, copy, geometry) == 0);
      copy.layers.back().sourceX = shown.layers.back().sourceX;
      REQUIRE_FALSE(sameLayout(shown, copy));
    }
  }

  GIVEN("A level whose panel palette changes between builds") {
    graphics::Display stage;
    stage.width = 304;
    stage.height = 255;
    stage.displayHeight = 255;
    stage.layers.push_back(layer(320, 222, 16, 13));
    stage.layers.back().columns = 304;
    graphics::Layer panel = layer(304, 32, 8, 14);
    panel.top = 223;
    stage.layers.push_back(panel);

    THEN("Each build uses the palettes it was given") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      graphics::Display shown = inArena(arena, stage);
      for (int pass = 0; pass < 3; ++pass) {
        BuiltFrame frame;
        buildFrame(shown, geometry, memory, nullptr, 0, frame);
        for (const Translation &translation : frame.translations) {
          translateOnCpu(translation);
        }
        REQUIRE(frame.copper == std::vector<uint32_t>{COPPER_END});
        REQUIRE(wrongPixels(arena, frame, memory, shown, geometry) == 0);
        shown.layers[1].palette[2] =
            static_cast<uint16_t>(shown.layers[1].palette[2] ^ 0x0F0);
        shown.layers[0].sourceX += 2;
      }
    }
  }
}

namespace {

const uint8_t *arenaCopy(Arena &arena, const uint8_t *pixels,
                         std::size_t size) {
  uint8_t *copy = arena.allocate(size);
  std::memcpy(copy, pixels, size);
  return copy;
}

int scrolledMismatches(Arena &arena, const FrameMemory &memory,
                       const graphics::Display &built, const BuiltFrame &frame,
                       const graphics::Display &next, const Geometry &geometry,
                       bool &patched) {
  BuiltFrame copy = frame;
  patched = scrollFrame(next, built, geometry, memory, copy);
  for (const Translation &translation : copy.translations) {
    translateOnCpu(translation);
  }
  return wrongPixels(arena, copy, memory, next, geometry);
}

} // namespace

SCENARIO("Scrolled frames are patched instead of rebuilt") {
  GIVEN("A panning picture over a rainbow") {
    graphics::Display display;
    display.width = 368;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer picture = layer(1008, 256, 32, 16);
    picture.wrap = true;
    picture.columns = 368;
    for (int row = 0; row < 223; ++row) {
      picture.rowColors.push_back(
          {row, 0, static_cast<uint16_t>((row * 0x13) & 0xFFF)});
    }
    display.layers.push_back(picture);

    THEN("Every new offset is patched and matches the desktop") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        ArenaBuffers buffers(arena);
        const FrameMemory memory = frameMemory(arena, buffers);
        graphics::Display built = inArena(arena, display);
        BuiltFrame frame;
        buildFrame(built, geometry, memory, nullptr, 0, frame);
        for (int offset : {1, 7, 300, 1000}) {
          graphics::Display next = built;
          next.layers[0].sourceX = offset;
          bool patched = false;
          REQUIRE(scrolledMismatches(arena, memory, built, frame, next,
                                     geometry, patched) == 0);
          REQUIRE(patched);
        }
      }
    }
  }

  GIVEN("A level whose play screen flips buffers and whose panel moves") {
    graphics::Display stage;
    stage.width = 304;
    stage.height = 255;
    stage.displayHeight = 255;
    stage.layers.push_back(layer(640, 222, 16, 17));
    stage.layers.back().columns = 304;
    graphics::Layer panel = layer(304, 32, 8, 18);
    panel.top = 223;
    stage.layers.push_back(panel);
    const graphics::Layer otherPlay = layer(640, 222, 16, 19);
    const graphics::Layer otherPanel = layer(304, 32, 8, 20);

    THEN("The patched frame shows the new buffers and offsets") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        ArenaBuffers buffers(arena);
        const FrameMemory memory = frameMemory(arena, buffers);
        graphics::Display built = inArena(arena, stage);
        BuiltFrame frame;
        buildFrame(built, geometry, memory, nullptr, 0, frame);
        for (const Translation &translation : frame.translations) {
          translateOnCpu(translation);
        }
        REQUIRE(frame.translations.size() == 1);
        graphics::Display next = built;
        next.layers[0].pixels = arenaCopy(arena, otherPlay.pixels, 640 * 222);
        next.layers[0].sourceX = 13;
        bool patched = false;
        REQUIRE(scrolledMismatches(arena, memory, built, frame, next, geometry,
                                   patched) == 0);
        REQUIRE(patched);
        next.layers[1].pixels = arenaCopy(arena, otherPanel.pixels, 304 * 32);
        REQUIRE(scrolledMismatches(arena, memory, built, frame, next, geometry,
                                   patched) == 0);
        REQUIRE(patched);
      }
    }

    THEN("A frame patched again and again stays correct") {
      graphics::Display tall = stage;
      tall.layers[0] = layer(640, 230, 16, 22);
      tall.layers[0].columns = 304;
      tall.layers[0].rows = 222;
      const graphics::Layer otherTall = layer(640, 230, 16, 23);
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        ArenaBuffers buffers(arena);
        const FrameMemory memory = frameMemory(arena, buffers);
        graphics::Display shown = inArena(arena, tall);
        const uint8_t *first = shown.layers[0].pixels;
        const uint8_t *second = arenaCopy(arena, otherTall.pixels, 640 * 230);
        BuiltFrame frame;
        buildFrame(shown, geometry, memory, nullptr, 0, frame);
        for (int step = 1; step <= 6; ++step) {
          graphics::Display next = shown;
          next.layers[0].pixels = step % 2 != 0 ? second : first;
          next.layers[0].sourceX = (step * 37) % 300;
          next.layers[0].sourceY = step;
          REQUIRE(scrollFrame(next, shown, geometry, memory, frame));
          for (const Translation &translation : frame.translations) {
            translateOnCpu(translation);
          }
          REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
          shown = next;
        }
      }
    }

    THEN("A changed palette is rebuilt instead") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display built = inArena(arena, stage);
      BuiltFrame frame;
      buildFrame(built, geometry, memory, nullptr, 0, frame);
      graphics::Display next = built;
      next.layers[1].palette[2] = 0x0F0;
      BuiltFrame copy = frame;
      REQUIRE_FALSE(scrollFrame(next, built, geometry, memory, copy));
    }
  }

  GIVEN("A hires picture") {
    graphics::Display display;
    display.width = 640;
    display.height = 200;
    display.displayHeight = 200;
    graphics::Layer picture = layer(800, 200, 16, 21);
    picture.columns = 640;
    display.layers.push_back(picture);

    THEN("Scaled objects are patched too") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display built = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(built, geometry, memory, nullptr, 0, frame);
      graphics::Display next = built;
      next.layers[0].sourceX = 33;
      bool patched = false;
      REQUIRE(scrolledMismatches(arena, memory, built, frame, next, geometry,
                                 patched) == 0);
      REQUIRE(patched);
    }
  }
}
