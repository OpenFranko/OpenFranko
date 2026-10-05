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

constexpr int SCALED_DONE = 0x3FF;

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
      const bool showing = scaled ? height != SCALED_DONE : height > 0;
      if (halfLine >= ypos && showing) {
        drawBitmap(arena, list, at, scaled, line, clut);
        uint32_t data = static_cast<uint32_t>(bits(phrase, 43, 21));
        const uint32_t dataWidth =
            static_cast<uint32_t>(bits(list[at + 1], 18, 10));
        if (scaled) {
          const int verticalScale = static_cast<int>(bits(list[at + 2], 8, 8));
          int remainder = static_cast<int>(bits(list[at + 2], 16, 8)) - 32;
          while (remainder <= 0 && height != SCALED_DONE) {
            remainder += verticalScale;
            if (height == 0) {
              height = SCALED_DONE;
            } else {
              --height;
              data += dataWidth;
            }
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
  uint8_t *mask = arena.allocate(8);
  return {arena.address(live),
          arena.address(solid),
          &buffers,
          lines,
          LINE_PHRASES,
          mask};
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
    for (graphics::Sprite &sprite : layer.sprites) {
      const std::size_t size =
          static_cast<std::size_t>(sprite.width * sprite.height);
      uint8_t *copy =
          arena.allocate(size, reinterpret_cast<uintptr_t>(sprite.pixels) % 8);
      std::memcpy(copy, sprite.pixels, size);
      sprite.pixels = copy;
    }
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

namespace {

void forgetBanks(Arena &arena, const Geometry &geometry,
                 const FrameMemory &memory) {
  graphics::Display other;
  other.width = 64;
  other.height = 48;
  other.displayHeight = 48;
  for (int index = 0; index < 3; ++index) {
    graphics::Layer part = layer(64, 16, 8, static_cast<uint8_t>(40 + index));
    part.top = index * 16;
    part.rows = 16;
    other.layers.push_back(part);
  }
  BuiltFrame scratch;
  buildFrame(inArena(arena, other), geometry, memory, nullptr, 0, scratch);
}

std::array<uint16_t, 256> freshClut(Arena &arena,
                                    const graphics::Display &display,
                                    const Geometry &geometry,
                                    const FrameMemory &memory) {
  forgetBanks(arena, geometry, memory);
  BuiltFrame fresh;
  buildFrame(display, geometry, memory, nullptr, 0, fresh);
  return fresh.clut;
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

  GIVEN("A play screen that shakes over a panel") {
    graphics::Display stage;
    stage.width = 304;
    stage.height = 255;
    stage.displayHeight = 255;
    graphics::Layer play = layer(320, 222, 16, 17);
    play.columns = 304;
    play.rows = 255;
    play.sourceX = 16;
    graphics::Layer panel = layer(304, 32, 8, 18);
    panel.top = 223;
    stage.layers = {play, panel};

    THEN("Moving it up and down is patched") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        ArenaBuffers buffers(arena);
        const FrameMemory memory = frameMemory(arena, buffers);
        const graphics::Display built = inArena(arena, stage);
        BuiltFrame frame;
        buildFrame(built, geometry, memory, nullptr, 0, frame);
        for (const Translation &translation : frame.translations) {
          translateOnCpu(translation);
        }
        for (const int shake : {-3, 2, -8, 5}) {
          graphics::Display next = built;
          next.layers[0].sourceY = shake;
          bool patched = false;
          REQUIRE(scrolledMismatches(arena, memory, built, frame, next,
                                     geometry, patched) == 0);
          REQUIRE(patched);
        }
      }
    }
  }

  GIVEN("Two pictures where the taller one owns the colours") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer upper = layer(320, 120, 64, 5);
    upper.rows = 120;
    graphics::Layer lower = layer(320, 130, 64, 9);
    lower.top = 120;
    lower.rows = 136;
    display.layers = {upper, lower};

    THEN("A move that keeps it taller is patched, one that does not is "
         "rebuilt") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display built = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(built, geometry, memory, nullptr, 0, frame);
      for (const Translation &translation : frame.translations) {
        translateOnCpu(translation);
      }
      graphics::Display next = built;
      next.layers[1].sourceY = 5;
      bool patched = false;
      REQUIRE(scrolledMismatches(arena, memory, built, frame, next, geometry,
                                 patched) == 0);
      REQUIRE(patched);
      next.layers[1].sourceY = 20;
      BuiltFrame copy = frame;
      REQUIRE_FALSE(scrollFrame(next, built, geometry, memory, copy));
    }
  }

  GIVEN("A picture with background line colours") {
    graphics::Display display;
    display.width = 368;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer picture = layer(368, 200, 32, 16);
    picture.rows = 256;
    for (int row = 0; row < 200; ++row) {
      picture.rowColors.push_back(
          {row, 0, static_cast<uint16_t>((row * 0x13) & 0xFFF)});
    }
    display.layers = {picture};

    THEN("Moving it up or down is rebuilt") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display built = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(built, geometry, memory, nullptr, 0, frame);
      REQUIRE(frame.lineLayer == 0);
      graphics::Display next = built;
      next.layers[0].sourceY = -10;
      BuiltFrame copy = frame;
      REQUIRE_FALSE(scrollFrame(next, built, geometry, memory, copy));
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

    THEN("A new panel colour during a flip is patched and recoloured") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, geometry, memory);
      const graphics::Display built = inArena(arena, stage);
      BuiltFrame frame;
      buildFrame(built, geometry, memory, nullptr, 0, frame);
      REQUIRE(frame.translations.size() == 1);
      graphics::Display next = built;
      next.layers[0].pixels = arenaCopy(arena, otherPlay.pixels, 640 * 222);
      next.layers[0].sourceX = 13;
      next.layers[1].palette[2] = 0x0F0;
      REQUIRE(scrollFrame(next, built, geometry, memory, frame));
      for (const Translation &translation : frame.translations) {
        translateOnCpu(translation);
      }
      REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
      REQUIRE(freshClut(arena, next, geometry, memory) == frame.clut);
    }

    THEN("Colours that need other banks are rebuilt instead") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, geometry, memory);
      graphics::Display shared = inArena(arena, stage);
      std::copy_n(shared.layers[0].palette.begin(),
                  shared.layers[1].palette.size(),
                  shared.layers[1].palette.begin());
      BuiltFrame frame;
      buildFrame(shared, geometry, memory, nullptr, 0, frame);
      REQUIRE(frame.translations.empty());
      graphics::Display next = shared;
      next.layers[0].pixels = arenaCopy(arena, otherPlay.pixels, 640 * 222);
      next.layers[1].palette = stage.layers[1].palette;
      REQUIRE_FALSE(scrollFrame(next, shared, geometry, memory, frame));
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

namespace {

struct Bob {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> pixels;
};

Bob bob(int width, int height, int seed) {
  Bob made;
  made.width = width;
  made.height = height;
  made.pixels.assign(static_cast<std::size_t>(width * height), 0);
  for (int at = 0; at < width * height; ++at) {
    const int value = at * 5 + seed;
    made.pixels[static_cast<std::size_t>(at)] =
        static_cast<uint8_t>(value % 4 == 0 ? 0 : 1 + value % 15);
  }
  return made;
}

using Places = std::vector<std::pair<int, int>>;

int borderLeaks(const Arena &arena, const BuiltFrame &frame,
                const FrameMemory &memory, const graphics::Display &display,
                const Geometry &geometry) {
  const Screen screen = simulate(arena, frame, memory.liveAddress, geometry);
  const Placement placement = placeDisplay(display, geometry);
  const int right = placement.left + display.width / placement.halfWidth;
  int leaks = 0;
  for (int row = 0; row < display.height; row += placement.rowsPerLine) {
    const int screenRow = row / placement.rowsPerLine + placement.top;
    if (screenRow < 0 || screenRow >= geometry.rows) {
      continue;
    }
    for (int column = 0; column < geometry.columns; ++column) {
      if ((column < placement.left || column >= right) &&
          screen[static_cast<std::size_t>(screenRow)]
                [static_cast<std::size_t>(column)] != frame.background) {
        ++leaks;
      }
    }
  }
  return leaks;
}

graphics::Display withBobs(graphics::Display display,
                           const std::vector<const Bob *> &shapes,
                           const Places &places) {
  graphics::Layer &screen = display.layers.front();
  screen.carriesSprites = true;
  screen.sprites.clear();
  for (std::size_t index = 0; index < shapes.size(); ++index) {
    const Bob &shape = *shapes[index];
    screen.sprites.push_back({shape.pixels.data(),
                              static_cast<int16_t>(shape.width),
                              static_cast<int16_t>(shape.height),
                              places[index].first, places[index].second});
  }
  return display;
}

graphics::Display stageDisplay(bool laced) {
  const int perLine = laced ? 2 : 1;
  graphics::Display display;
  display.width = 304;
  display.height = 255 * perLine;
  display.displayHeight = 255;
  graphics::Layer screen = layer(320, 222 * perLine, 16, 17);
  screen.columns = 304;
  screen.rows = 255 * perLine;
  screen.sourceX = 16;
  screen.sourceY = -3 * perLine;
  graphics::Layer panel = layer(304, 32, 8, 18);
  panel.top = 223 * perLine;
  panel.rows = 32 * perLine;
  panel.repeat = perLine;
  display.layers = {screen, panel};
  return display;
}

graphics::Display hiresDisplay() {
  graphics::Display display;
  display.width = 640;
  display.height = 200;
  display.displayHeight = 200;
  graphics::Layer picture = layer(800, 200, 16, 21);
  picture.columns = 640;
  picture.sourceX = 40;
  display.layers = {picture};
  return display;
}

} // namespace

SCENARIO("Bitmap objects are packed into the object processor's layout") {
  GIVEN("Objects with every field set") {
    std::vector<BitmapObject> objects;
    for (int seed = 0; seed < 64; ++seed) {
      BitmapObject object;
      object.data = static_cast<uint32_t>(0x1234568u * (seed + 3)) & 0xFFFFF8u;
      object.firstPixel = (seed * 5) & 0x3F;
      object.x = (seed * 37 - 200) & 0xFFF;
      object.y = (seed * 91 + 7) & 0x7FF;
      object.height = (seed * 13 + 1) & 0x3FF;
      object.dataWidth = (seed * 29 + 3) & 0x3FF;
      object.imageWidth = (seed * 41 + 5) & 0x3FF;
      object.pitch = seed & 7;
      object.depth = static_cast<Depth>(seed % 5);
      object.index = (seed * 3) & 0x7F;
      object.transparent = (seed & 1) != 0;
      object.reflected = (seed & 2) != 0;
      object.released = (seed & 4) != 0;
      object.scaled = (seed & 8) != 0;
      object.horizontalScale = static_cast<uint8_t>(seed * 7 + 1);
      object.verticalScale = static_cast<uint8_t>(seed * 11 + 2);
      objects.push_back(object);
    }

    THEN("Each field lands in its bits") {
      for (std::size_t at = 0; at < objects.size(); ++at) {
        const BitmapObject &object = objects[at];
        const uint32_t link = static_cast<uint32_t>(0x7654320u + at * 0x88u);
        uint64_t out[3] = {};
        bitmapPhrases(object, link, out);
        REQUIRE(bits(out[0], 0, 3) == (object.scaled ? 1u : 0u));
        REQUIRE(bits(out[0], 3, 11) == static_cast<uint64_t>(object.y));
        REQUIRE(bits(out[0], 14, 10) == static_cast<uint64_t>(object.height));
        REQUIRE(bits(out[0], 24, 19) == ((link >> 3) & 0x7FFFF));
        REQUIRE(bits(out[0], 43, 21) == object.data >> 3);
        REQUIRE(bits(out[1], 0, 12) == static_cast<uint64_t>(object.x));
        REQUIRE(bits(out[1], 12, 3) == static_cast<uint64_t>(object.depth));
        REQUIRE(bits(out[1], 15, 3) == static_cast<uint64_t>(object.pitch));
        REQUIRE(bits(out[1], 18, 10) ==
                static_cast<uint64_t>(object.dataWidth));
        REQUIRE(bits(out[1], 28, 10) ==
                static_cast<uint64_t>(object.imageWidth));
        REQUIRE(bits(out[1], 38, 7) == static_cast<uint64_t>(object.index));
        REQUIRE(bits(out[1], 45, 1) == (object.reflected ? 1u : 0u));
        REQUIRE(bits(out[1], 46, 1) == 0);
        REQUIRE(bits(out[1], 47, 1) == (object.transparent ? 1u : 0u));
        REQUIRE(bits(out[1], 48, 1) == (object.released ? 1u : 0u));
        REQUIRE(bits(out[1], 49, 6) ==
                static_cast<uint64_t>(object.firstPixel));
        REQUIRE(bits(out[1], 55, 9) == 0);
        if (object.scaled) {
          REQUIRE(bits(out[2], 0, 8) == object.horizontalScale);
          REQUIRE(bits(out[2], 8, 8) == object.verticalScale);
          REQUIRE(bits(out[2], 16, 8) == object.verticalScale);
          REQUIRE(bits(out[2], 24, 40) == 0);
        }
      }
    }
  }
}

SCENARIO("A sprite object can be rewritten without a bitmap object") {
  GIVEN("Unscaled transparent objects already linked into a list") {
    THEN("The direct rewrite matches the general one and keeps the link") {
      for (int seed = 0; seed < 64; ++seed) {
        BitmapObject object;
        object.data = static_cast<uint32_t>(0x123458u * (seed + 1)) & 0x3FFFF8u;
        object.x = (seed * 37 - 100) & 0xFFF;
        object.y = (seed * 91 + 3) & 0x7FF;
        object.height = (seed * 13 + 1) & 0x3FF;
        object.dataWidth = (seed * 7 + 2) & 0x3FF;
        object.imageWidth = (seed * 41 + 5) & 0x3FF;
        object.firstPixel = (seed * 8) & 0x38;
        object.transparent = true;
        const uint32_t link = static_cast<uint32_t>(0x654320u + seed * 0x40u);
        uint64_t general[2] = {};
        bitmapPhrases(BitmapObject{}, link, general);
        uint64_t direct[2] = {general[0], general[1]};
        rewriteBitmap(object, general);
        rewriteSprite(direct, object.data, object.x, object.y, object.height,
                      object.dataWidth, object.imageWidth, object.firstPixel);
        REQUIRE(direct[0] == general[0]);
        REQUIRE(direct[1] == general[1]);
      }
    }
  }
}

namespace {

constexpr int PLACING_CHANGES = 15;

void changePlacing(graphics::Layer &layer, int change) {
  switch (change) {
  case 0:
    ++layer.pixels;
    break;
  case 1:
    ++layer.sourceX;
    break;
  case 2:
    ++layer.sourceY;
    break;
  case 3:
    ++layer.stride;
    break;
  case 4:
    ++layer.sourceColumns;
    break;
  case 5:
    ++layer.sourceRows;
    break;
  case 6:
    ++layer.sourceStep;
    break;
  case 7:
    ++layer.repeat;
    break;
  case 8:
    layer.wrap = !layer.wrap;
    break;
  case 9:
    ++layer.left;
    break;
  case 10:
    ++layer.top;
    break;
  case 11:
    ++layer.columns;
    break;
  case 12:
    ++layer.rows;
    break;
  case 13:
    layer.mask = 0x0F;
    break;
  default:
    layer.carriesSprites = !layer.carriesSprites;
    break;
  }
}

} // namespace

SCENARIO("Displays are placed alike only when the screen and every layer "
         "match") {
  GIVEN("A display with two layers") {
    const graphics::Display display = stageDisplay(false);

    THEN("A copy with other colours, revisions and sprites is placed alike") {
      graphics::Display copy = display;
      copy.revision = 7;
      copy.layers[0].palette[1] ^= 0x111;
      copy.layers[1].revision = 9;
      copy.layers[1].sprites.push_back(graphics::Sprite{});
      REQUIRE(samePlacing(display, copy));
      REQUIRE(samePlacing(copy, display));
    }

    THEN("Any placing change in either layer is noticed both ways") {
      for (std::size_t index = 0; index < display.layers.size(); ++index) {
        for (int change = 0; change < PLACING_CHANGES; ++change) {
          graphics::Display moved = display;
          changePlacing(moved.layers[index], change);
          REQUIRE_FALSE(samePlacing(display, moved));
          REQUIRE_FALSE(samePlacing(moved, display));
        }
      }
    }

    THEN("A missing layer is noticed on either side") {
      graphics::Display shorter = display;
      shorter.layers.pop_back();
      REQUIRE_FALSE(samePlacing(display, shorter));
      REQUIRE_FALSE(samePlacing(shorter, display));
      graphics::Display empty = display;
      empty.layers.clear();
      REQUIRE_FALSE(samePlacing(display, empty));
      REQUIRE_FALSE(samePlacing(empty, display));
      REQUIRE(samePlacing(empty, empty));
    }

    THEN("Another screen size or border is noticed") {
      for (int change = 0; change < 4; ++change) {
        graphics::Display other = display;
        int *fields[] = {&other.width, &other.height, &other.displayHeight};
        if (change < 3) {
          ++*fields[change];
        } else {
          other.border = 0x123;
        }
        REQUIRE_FALSE(samePlacing(display, other));
        REQUIRE_FALSE(samePlacing(other, display));
      }
    }
  }
}

SCENARIO("A bitmap object can be pointed at other data") {
  GIVEN("Objects already linked into a list") {
    THEN("Only the data address changes") {
      for (int seed = 0; seed < 64; ++seed) {
        BitmapObject object;
        object.data = static_cast<uint32_t>(0x123458u * (seed + 1)) & 0xFFFFF8u;
        object.x = (seed * 37 - 100) & 0xFFF;
        object.y = (seed * 91 + 3) & 0x7FF;
        object.height = (seed * 13 + 1) & 0x3FF;
        object.dataWidth = (seed * 7 + 2) & 0x3FF;
        object.imageWidth = (seed * 41 + 5) & 0x3FF;
        object.depth = static_cast<Depth>(seed % 5);
        object.transparent = (seed & 1) != 0;
        object.scaled = (seed & 8) != 0;
        const uint32_t link = static_cast<uint32_t>(0x654320u + seed * 0x40u);
        uint64_t phrases[3] = {};
        bitmapPhrases(object, link, phrases);
        object.data = static_cast<uint32_t>(0x2468A8u * (seed + 5)) & 0xFFFFF8u;
        retargetBitmap(phrases, object.data);
        uint64_t expected[3] = {};
        bitmapPhrases(object, link, expected);
        REQUIRE(phrases[0] == expected[0]);
        REQUIRE(phrases[1] == expected[1]);
        REQUIRE(phrases[2] == expected[2]);
      }
    }
  }
}

SCENARIO("Sprites are shown as objects over the layer that carries them") {
  const Bob small = bob(16, 12, 1);
  const Bob wide = bob(48, 40, 2);
  const Bob tall = bob(32, 81, 3);
  const std::vector<const Bob *> shapes = {&wide, &small, &tall,
                                           &wide, &small, &tall};
  const Places places = {{100, 50},  {-5, 100}, {300, 180},
                         {150, -21}, {2000, 0}, {61, 211}};
  const std::vector<Places> moves = {
      {{101, 53}, {-3, 100}, {290, 185}, {150, -18}, {3000, 0}, {61, 200}},
      {{-60, 52}, {20, 21}, {290, 301}, {150, 10}, {40, 40}, {61, 200}},
      {{-60, 52}, {20, 21}, {120, 120}, {151, 11}, {40, 40}, {-80, 5}},
      {{10, 10}, {311, 213}, {120, 121}, {0, 0}, {41, 40}, {-30, 5}}};

  GIVEN("A stage screen, a panel and bobs on every edge") {
    THEN("The built frame matches the bobs drawn into the screen") {
      for (const bool laced : {false, true}) {
        const graphics::Display stage =
            withBobs(stageDisplay(laced), shapes, places);
        for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
          REQUIRE(mismatches(stage, geometry) == 0);
          Arena arena;
          const graphics::Display display = inArena(arena, stage);
          FrameMemory memory;
          const BuiltFrame frame = build(arena, display, geometry, memory);
          REQUIRE(borderLeaks(arena, frame, memory, display, geometry) == 0);
        }
      }
    }

    THEN("Moving, adding, removing and scrolling them only patches") {
      for (const bool laced : {false, true}) {
        const graphics::Display stage =
            withBobs(stageDisplay(laced), shapes, places);
        for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
          Arena arena;
          ArenaBuffers buffers(arena);
          const FrameMemory memory = frameMemory(arena, buffers);
          graphics::Display shown = inArena(arena, stage);
          BuiltFrame frame;
          buildFrame(shown, geometry, memory, nullptr, 0, frame);
          for (const Translation &translation : frame.translations) {
            translateOnCpu(translation);
          }
          REQUIRE(frame.spriteSlots.size() == 1);
          std::vector<const Bob *> swapped = shapes;
          std::size_t count = shapes.size();
          for (const Places &move : moves) {
            std::rotate(swapped.begin(), swapped.begin() + 1, swapped.end());
            count = count == shapes.size() ? 2 : count + 2;
            const std::vector<const Bob *> some(swapped.begin(),
                                                swapped.begin() + count);
            graphics::Display next =
                inArena(arena, withBobs(stage, some, move));
            next.layers.front().pixels = shown.layers.front().pixels;
            next.layers.back().pixels = shown.layers.back().pixels;
            REQUIRE(scrollFrame(next, shown, geometry, memory, frame));
            REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
            REQUIRE(borderLeaks(arena, frame, memory, next, geometry) == 0);
            shown = next;
            next.layers.front().sourceX -= 3;
            REQUIRE(scrollFrame(next, shown, geometry, memory, frame));
            REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
            REQUIRE(borderLeaks(arena, frame, memory, next, geometry) == 0);
            shown = next;
          }
        }
      }
    }
  }

  GIVEN("A built frame whose screen keeps still while its bobs move") {
    THEN("Only the sprite objects are rewritten") {
      for (const bool laced : {false, true}) {
        const graphics::Display stage =
            withBobs(stageDisplay(laced), shapes, places);
        const Geometry geometry = palGeometry();
        Arena arena;
        ArenaBuffers buffers(arena);
        const FrameMemory memory = frameMemory(arena, buffers);
        graphics::Display shown = inArena(arena, stage);
        BuiltFrame frame;
        buildFrame(shown, geometry, memory, nullptr, 0, frame);
        for (const Translation &translation : frame.translations) {
          translateOnCpu(translation);
        }
        const std::vector<uint64_t> before = frame.phrases;
        for (const Places &move : moves) {
          graphics::Display next =
              inArena(arena, withBobs(stage, shapes, move));
          next.layers.front().pixels = shown.layers.front().pixels;
          next.layers.back().pixels = shown.layers.back().pixels;
          REQUIRE(sameLayers(next, shown));
          REQUIRE_FALSE(sameSprites(next, shown));
          SpriteLists wanted;
          for (const graphics::Layer &layer : next.layers) {
            wanted.push_back(layer.sprites);
          }
          REQUIRE(sameSprites(next, wanted));
          REQUIRE_FALSE(sameSprites(shown, wanted));
          if (&move == &moves.front() || &move == &moves.back()) {
            REQUIRE(moveSprites(next, shown, geometry, memory, frame));
          } else {
            REQUIRE(moveSprites(shown, wanted, geometry, memory, frame));
          }
          REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
          REQUIRE(borderLeaks(arena, frame, memory, next, geometry) == 0);
          shown = next;
        }
        for (std::size_t at = 0; at < before.size(); ++at) {
          bool sprite = false;
          for (const int object : frame.spriteSlots.front().objects) {
            const std::size_t first = static_cast<std::size_t>(object);
            sprite = sprite || (at >= first && at < first + 2);
          }
          if (!sprite) {
            REQUIRE(frame.phrases[at] == before[at]);
          }
        }
      }
    }
  }

  GIVEN("A hires picture carrying bobs") {
    const graphics::Display picture = withBobs(
        hiresDisplay(), shapes,
        {{100, 50}, {-6, 100}, {790, 180}, {150, -20}, {42, 7}, {600, 150}});

    THEN("Scaled sprite objects are built and patched") {
      const Geometry geometry = palGeometry();
      REQUIRE(mismatches(picture, geometry) == 0);
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display shown = inArena(arena, picture);
      BuiltFrame frame;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      graphics::Display next = shown;
      next.layers.front().sprites[0].left += 10;
      next.layers.front().sprites[3].top += 5;
      next.layers.front().sprites.pop_back();
      REQUIRE(scrollFrame(next, shown, geometry, memory, frame));
      REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
    }
  }

  GIVEN("A taller backdrop under a smaller layer carrying bobs") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer screen = layer(320, 120, 64, 9);
    screen.top = 60;
    screen.carriesSprites = true;
    screen.sprites = {{wide.pixels.data(), static_cast<int16_t>(wide.width),
                       static_cast<int16_t>(wide.height), 100, 30},
                      {small.pixels.data(), static_cast<int16_t>(small.width),
                       static_cast<int16_t>(small.height), 200, 90}};
    display.layers = {layer(320, 256, 64, 5), screen};

    THEN("The carrying layer keeps its colours native for the bobs") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }
  }

  GIVEN("A layer whose pixels end inside the display") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer screen = layer(200, 100, 16, 7);
    screen.columns = 320;
    screen.rows = 256;
    screen.carriesSprites = true;
    screen.sprites = {{wide.pixels.data(), static_cast<int16_t>(wide.width),
                       static_cast<int16_t>(wide.height), 184, 20},
                      {tall.pixels.data(), static_cast<int16_t>(tall.width),
                       static_cast<int16_t>(tall.height), 40, 60}};
    display.layers = {screen};

    THEN("Bobs are cut where the pixels end") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }
  }

  GIVEN("Two displays that differ only in their sprites") {
    const graphics::Display first =
        withBobs(stageDisplay(false), shapes, places);
    graphics::Display second = first;
    second.layers.front().sprites[2].left += 1;

    THEN("Their layouts differ") {
      REQUIRE(sameLayout(first, first));
      REQUIRE_FALSE(sameLayout(first, second));
    }
  }
}

SCENARIO("A frame copied into another slot keeps working there") {
  const Bob small = bob(16, 12, 1);
  const Bob wide = bob(48, 40, 2);
  const std::vector<const Bob *> shapes = {&wide, &small};
  const Places places = {{-20, 60}, {296, 120}};

  GIVEN("A stage frame with bobs over both edges") {
    THEN("The copy keeps its own masks and shakes like the original") {
      for (const bool laced : {false, true}) {
        graphics::Display stage = withBobs(stageDisplay(laced), shapes, places);
        stage.border = 0x555;
        for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
          Arena arena;
          ArenaBuffers buffers(arena);
          const FrameMemory first = frameMemory(arena, buffers);
          FrameMemory second = first;
          second.maskPhrase = arena.allocate(8);
          const graphics::Display shown = inArena(arena, stage);
          BuiltFrame frame;
          buildFrame(shown, geometry, first, nullptr, 0, frame);
          for (const Translation &translation : frame.translations) {
            translateOnCpu(translation);
          }
          REQUIRE(frame.masks.size() == 2);
          BuiltFrame copy;
          REQUIRE(copyFrame(frame, second, copy));
          std::memset(first.maskPhrase, 0x5A, 8);
          REQUIRE(wrongPixels(arena, copy, second, shown, geometry) == 0);
          REQUIRE(borderLeaks(arena, copy, second, shown, geometry) == 0);
          graphics::Display next = shown;
          next.layers.front().sourceY -= 8;
          REQUIRE(scrollFrame(next, shown, geometry, second, copy));
          REQUIRE(wrongPixels(arena, copy, second, next, geometry) == 0);
          REQUIRE(borderLeaks(arena, copy, second, next, geometry) == 0);
        }
      }
    }
  }

  GIVEN("A frame whose colours come from line phrases") {
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

    THEN("It is not copied") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display built = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(built, palGeometry(), memory, nullptr, 0, frame);
      REQUIRE(frame.lineObject != -1);
      BuiltFrame copy;
      REQUIRE_FALSE(copyFrame(frame, memory, copy));
    }
  }
}

SCENARIO("Frames whose colours change are recoloured instead of rebuilt") {
  GIVEN("A picture fading out") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    display.layers.push_back(layer(320, 256, 32, 4));

    THEN("Only the colour table changes, as a rebuild would make it") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        ArenaBuffers buffers(arena);
        const FrameMemory memory = frameMemory(arena, buffers);
        const graphics::Display built = inArena(arena, display);
        BuiltFrame frame;
        buildFrame(built, geometry, memory, nullptr, 0, frame);
        const std::vector<uint64_t> phrases = frame.phrases;
        graphics::Display faded = built;
        for (uint16_t &color : faded.layers[0].palette) {
          color = static_cast<uint16_t>((color >> 1) & 0x777);
        }
        REQUIRE(recolorFrame(faded, built, geometry, memory, frame));
        REQUIRE(frame.phrases == phrases);
        REQUIRE(wrongPixels(arena, frame, memory, faded, geometry) == 0);
        BuiltFrame rebuilt;
        buildFrame(faded, geometry, memory, nullptr, 0, rebuilt);
        REQUIRE(rebuilt.clut == frame.clut);
      }
    }

    THEN("A frame placed for another screen is left to a rebuild") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display built = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(built, palGeometry(), memory, nullptr, 0, frame);
      graphics::Display faded = built;
      faded.layers[0].palette[1] = 0x0F0;
      REQUIRE_FALSE(recolorFrame(faded, built, ntscGeometry(), memory, frame));
    }

    THEN("A palette of another size is left to a rebuild") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display built = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(built, palGeometry(), memory, nullptr, 0, frame);
      graphics::Display longer = built;
      longer.layers[0].palette.push_back(0x123);
      REQUIRE_FALSE(recolorFrame(longer, built, palGeometry(), memory, frame));
    }

    THEN("Row colours are left to a rebuild") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display built = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(built, palGeometry(), memory, nullptr, 0, frame);
      graphics::Display banded = built;
      banded.layers[0].rowColors.push_back({10, 1, 0xF00});
      REQUIRE_FALSE(recolorFrame(banded, built, palGeometry(), memory, frame));
    }
  }

  GIVEN("A play screen over a panel whose colours need their own bank") {
    graphics::Display stage;
    stage.width = 304;
    stage.height = 255;
    stage.displayHeight = 255;
    stage.layers.push_back(layer(320, 222, 16, 2));
    stage.layers.back().columns = 304;
    graphics::Layer panel = layer(304, 32, 8, 3);
    panel.top = 223;
    stage.layers.push_back(panel);

    THEN("New colours that keep the banks only change the colour table") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        ArenaBuffers buffers(arena);
        const FrameMemory memory = frameMemory(arena, buffers);
        const graphics::Display built = inArena(arena, stage);
        BuiltFrame frame;
        buildFrame(built, geometry, memory, nullptr, 0, frame);
        for (const Translation &translation : frame.translations) {
          translateOnCpu(translation);
        }
        REQUIRE_FALSE(frame.translations.empty());
        const std::vector<uint64_t> phrases = frame.phrases;
        graphics::Display faded = built;
        for (graphics::Layer &layer : faded.layers) {
          for (uint16_t &color : layer.palette) {
            color = static_cast<uint16_t>((color >> 1) & 0x777);
          }
        }
        REQUIRE(recolorFrame(faded, built, geometry, memory, frame));
        REQUIRE(frame.phrases == phrases);
        REQUIRE(wrongPixels(arena, frame, memory, faded, geometry) == 0);
      }
    }

    THEN("Colours that would let the panel share the first bank keep its "
         "own") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, palGeometry(), memory);
      const graphics::Display built = inArena(arena, stage);
      BuiltFrame frame;
      buildFrame(built, palGeometry(), memory, nullptr, 0, frame);
      for (const Translation &translation : frame.translations) {
        translateOnCpu(translation);
      }
      REQUIRE_FALSE(frame.translations.empty());
      const std::vector<uint64_t> phrases = frame.phrases;
      graphics::Display shared = built;
      std::copy_n(shared.layers[0].palette.begin(),
                  shared.layers[1].palette.size(),
                  shared.layers[1].palette.begin());
      REQUIRE(recolorFrame(shared, built, palGeometry(), memory, frame));
      REQUIRE(frame.phrases == phrases);
      REQUIRE(wrongPixels(arena, frame, memory, shared, palGeometry()) == 0);
    }

    THEN("A panel that stops sharing the first bank gets its own again") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, palGeometry(), memory);
      graphics::Display shared = inArena(arena, stage);
      std::copy_n(shared.layers[0].palette.begin(),
                  shared.layers[1].palette.size(),
                  shared.layers[1].palette.begin());
      BuiltFrame frame;
      buildFrame(shared, palGeometry(), memory, nullptr, 0, frame);
      REQUIRE(frame.translations.empty());
      graphics::Display apart = shared;
      apart.layers[1].palette = stage.layers[1].palette;
      if (!recolorFrame(apart, shared, palGeometry(), memory, frame)) {
        buildFrame(apart, palGeometry(), memory, nullptr, 0, frame);
      }
      for (const Translation &translation : frame.translations) {
        translateOnCpu(translation);
      }
      REQUIRE(wrongPixels(arena, frame, memory, apart, palGeometry()) == 0);
      REQUIRE(freshClut(arena, apart, palGeometry(), memory) == frame.clut);
    }
  }

  GIVEN("Text fading in over a dancer, in a bank of its own") {
    graphics::Display credits;
    credits.width = 320;
    credits.height = 256;
    credits.displayHeight = 256;
    graphics::Layer dancer = layer(320, 164, 16, 6);
    dancer.top = 81;
    dancer.mask = 15;
    graphics::Layer text = layer(320, 80, 16, 8);
    text.mask = 15;
    credits.layers = {dancer, text};

    THEN("Every step gives the colour table a fresh plan would") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        Arena arena;
        ArenaBuffers buffers(arena);
        const FrameMemory memory = frameMemory(arena, buffers);
        graphics::Display shown = inArena(arena, credits);
        BuiltFrame frame;
        buildFrame(shown, geometry, memory, nullptr, 0, frame);
        for (const Translation &translation : frame.translations) {
          translateOnCpu(translation);
        }
        REQUIRE(frame.translations.size() == 1);
        const std::vector<uint64_t> phrases = frame.phrases;
        for (int step = 1; step <= 15; ++step) {
          graphics::Display next = shown;
          next.layers[1].palette[1] = static_cast<uint16_t>(step * 0x111);
          next.layers[1].palette[2] = static_cast<uint16_t>(step * 0x011);
          REQUIRE(recolorFrame(next, shown, geometry, memory, frame));
          REQUIRE(frame.phrases == phrases);
          REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
          REQUIRE(freshClut(arena, next, geometry, memory) == frame.clut);
          shown = next;
        }
      }
    }
  }

  GIVEN("Text fading over a dancer, recoloured in place") {
    graphics::Display credits;
    credits.width = 320;
    credits.height = 256;
    credits.displayHeight = 256;
    graphics::Layer dancer = layer(320, 164, 16, 6);
    dancer.top = 81;
    dancer.mask = 15;
    graphics::Layer text = layer(320, 80, 16, 8);
    text.mask = 15;
    credits.layers = {dancer, text};

    THEN("Both frames of the pair take the new colours without other work") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, geometry, memory);
      const graphics::Display shown = inArena(arena, credits);
      BuiltFrame frame;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      for (const Translation &translation : frame.translations) {
        translateOnCpu(translation);
      }
      BuiltFrame twin = frame;
      const std::vector<uint64_t> phrases = frame.phrases;
      graphics::Display next = shown;
      next.layers[1].palette[1] = 0x0F0;
      next.layers[1].palette[2] = 0x070;
      REQUIRE(recolorPalettes(next, frame));
      REQUIRE(frame.phrases == phrases);
      REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
      REQUIRE(recolorPalettes(next, twin));
      REQUIRE(twin.clut == frame.clut);
      REQUIRE(twin.clutVersion == frame.clutVersion);
      REQUIRE(freshClut(arena, next, geometry, memory) == frame.clut);
    }

    THEN("A frame from an older plan is left to the full path") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display shown = inArena(arena, credits);
      BuiltFrame frame;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      forgetBanks(arena, geometry, memory);
      BuiltFrame other;
      buildFrame(shown, geometry, memory, nullptr, 0, other);
      graphics::Display next = shown;
      next.layers[1].palette[1] = 0x0F0;
      REQUIRE_FALSE(recolorPalettes(next, frame));
      REQUIRE(recolorPalettes(next, other));
    }

    THEN("Row colours are left to the full path") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display shown = inArena(arena, credits);
      BuiltFrame frame;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      graphics::Display next = shown;
      next.layers[1].rowColors.push_back({10, 1, 0xF00});
      REQUIRE_FALSE(recolorPalettes(next, frame));
    }
  }

  GIVEN("A panel sharing the first bank with the play screen") {
    graphics::Display stage;
    stage.width = 304;
    stage.height = 255;
    stage.displayHeight = 255;
    stage.layers.push_back(layer(320, 222, 16, 2));
    stage.layers.back().columns = 304;
    graphics::Layer panel = layer(304, 32, 8, 3);
    panel.top = 223;
    std::copy_n(stage.layers[0].palette.begin(), panel.palette.size(),
                panel.palette.begin());
    stage.layers.push_back(panel);

    THEN("New panel colours are left to the full path") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, geometry, memory);
      const graphics::Display shown = inArena(arena, stage);
      BuiltFrame frame;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      REQUIRE(frame.translations.empty());
      graphics::Display next = shown;
      next.layers[1].palette[3] = 0xF0F;
      REQUIRE_FALSE(recolorPalettes(next, frame));
    }
  }

  GIVEN("A picture over a solid colour, recoloured in place") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer solid;
    solid.columns = 320;
    solid.rows = 256;
    solid.palette = {0x00F};
    graphics::Layer picture = layer(320, 100, 16, 5);
    picture.top = 50;
    display.layers = {solid, picture};

    THEN("A new solid colour is left to the full path") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, geometry, memory);
      const graphics::Display shown = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      graphics::Display next = shown;
      next.layers[0].palette[0] = 0x0F0;
      REQUIRE_FALSE(recolorPalettes(next, frame));
    }
  }

  GIVEN("A picture whose pixels carry bits above its colour mask") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer picture = layer(320, 256, 64, 4);
    picture.mask = 31;
    picture.palette.resize(32);
    display.layers.push_back(picture);

    THEN("Fading it recolours the unused slots too") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      graphics::Display shown = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(shown, palGeometry(), memory, nullptr, 0, frame);
      for (int step = 0; step < 4; ++step) {
        graphics::Display next = shown;
        for (uint16_t &color : next.layers[0].palette) {
          color = static_cast<uint16_t>((color >> 1) & 0x777);
        }
        REQUIRE(recolorFrame(next, shown, palGeometry(), memory, frame));
        REQUIRE(wrongPixels(arena, frame, memory, next, palGeometry()) == 0);
        REQUIRE(freshClut(arena, next, palGeometry(), memory) == frame.clut);
        shown = next;
      }
    }
  }

  GIVEN("A picture whose mask is not a run of low bits") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer picture = layer(320, 256, 64, 7);
    picture.mask = 23;
    picture.palette.resize(24);
    display.layers.push_back(picture);

    THEN("Fading it recolours every slot its colours reach") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, palGeometry(), memory);
      const graphics::Display shown = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(shown, palGeometry(), memory, nullptr, 0, frame);
      graphics::Display next = shown;
      next.layers[0].palette[5] = 0xF0F;
      next.layers[0].palette[18] = 0x0FF;
      REQUIRE(recolorFrame(next, shown, palGeometry(), memory, frame));
      REQUIRE(wrongPixels(arena, frame, memory, next, palGeometry()) == 0);
      REQUIRE(freshClut(arena, next, palGeometry(), memory) == frame.clut);
    }
  }

  GIVEN("A picture whose palette is longer than its mask") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer picture = layer(320, 256, 16, 3);
    picture.mask = 15;
    picture.palette.resize(32, 0x123);
    display.layers.push_back(picture);

    THEN("Colours past the mask change nothing") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, palGeometry(), memory);
      const graphics::Display shown = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(shown, palGeometry(), memory, nullptr, 0, frame);
      const std::array<uint16_t, 256> before = frame.clut;
      graphics::Display next = shown;
      for (std::size_t value = 16; value < 32; ++value) {
        next.layers[0].palette[value] = 0xFFF;
      }
      REQUIRE(recolorFrame(next, shown, palGeometry(), memory, frame));
      REQUIRE(frame.clut == before);
      REQUIRE(freshClut(arena, next, palGeometry(), memory) == frame.clut);
    }
  }

  GIVEN("Two masked pictures sharing the first bank") {
    graphics::Display display;
    display.width = 320;
    display.height = 200;
    display.displayHeight = 200;
    graphics::Layer upper = layer(320, 100, 64, 3);
    upper.palette.resize(16);
    upper.mask = 15;
    upper.carriesSprites = true;
    graphics::Layer lower = layer(320, 100, 64, 3);
    lower.palette.assign(upper.palette.begin(), upper.palette.begin() + 8);
    lower.mask = 7;
    lower.top = 100;
    display.layers = {upper, lower};

    THEN("The layer that takes over the sprites fills the unused slots") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display shown = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(shown, palGeometry(), memory, nullptr, 0, frame);
      REQUIRE(frame.translations.empty());
      graphics::Display next = shown;
      next.layers[0].carriesSprites = false;
      next.layers[1].carriesSprites = true;
      for (std::size_t value = 8; value < 16; ++value) {
        next.layers[0].palette[value] = static_cast<uint16_t>(0x0F0 + value);
      }
      if (!recolorFrame(next, shown, palGeometry(), memory, frame)) {
        buildFrame(next, palGeometry(), memory, nullptr, 0, frame);
      }
      REQUIRE(frame.translations.empty());
      REQUIRE(freshClut(arena, next, palGeometry(), memory) == frame.clut);
    }
  }

  GIVEN("Two pictures, the upper one carrying the sprites") {
    const Bob small = bob(16, 12, 1);
    graphics::Display display;
    display.width = 320;
    display.height = 220;
    display.displayHeight = 220;
    graphics::Layer upper = layer(320, 120, 16, 2);
    upper.mask = 15;
    upper.carriesSprites = true;
    graphics::Layer lower = layer(320, 100, 16, 9);
    lower.mask = 15;
    lower.top = 120;
    display.layers = {upper, lower};

    THEN("The lower one gets the first bank when it takes the sprites over") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, palGeometry(), memory);
      const graphics::Display shown = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(shown, palGeometry(), memory, nullptr, 0, frame);
      REQUIRE(frame.translations.size() == 1);
      graphics::Display moved = display;
      moved.layers[0].carriesSprites = false;
      moved.layers[1].carriesSprites = true;
      moved.layers[1].sprites.push_back(
          {small.pixels.data(), static_cast<int16_t>(small.width),
           static_cast<int16_t>(small.height), 40, 30});
      for (uint16_t &color : moved.layers[1].palette) {
        color = static_cast<uint16_t>((color >> 1) & 0x777);
      }
      const graphics::Display next = inArena(arena, moved);
      if (!recolorFrame(next, shown, palGeometry(), memory, frame)) {
        buildFrame(next, palGeometry(), memory, nullptr, 0, frame);
      }
      for (const Translation &translation : frame.translations) {
        translateOnCpu(translation);
      }
      REQUIRE(wrongPixels(arena, frame, memory, next, palGeometry()) == 0);
    }
  }

  GIVEN("A picture over a solid colour") {
    graphics::Display display;
    display.width = 320;
    display.height = 256;
    display.displayHeight = 256;
    graphics::Layer solid;
    solid.columns = 320;
    solid.rows = 256;
    solid.palette = {0x00F};
    graphics::Layer picture = layer(320, 100, 16, 5);
    picture.top = 50;
    display.layers = {solid, picture};

    THEN("A new solid colour gives the table a fresh plan would") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display shown = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(shown, palGeometry(), memory, nullptr, 0, frame);
      graphics::Display next = shown;
      next.layers[0].palette[0] = 0x0F0;
      if (!recolorFrame(next, shown, palGeometry(), memory, frame)) {
        buildFrame(next, palGeometry(), memory, nullptr, 0, frame);
      }
      REQUIRE(wrongPixels(arena, frame, memory, next, palGeometry()) == 0);
      REQUIRE(freshClut(arena, next, palGeometry(), memory) == frame.clut);
    }
  }
}

namespace {

graphics::Display fullScreen() {
  graphics::Display display;
  display.width = 320;
  display.height = 256;
  display.displayHeight = 256;
  return display;
}

graphics::Display bandedStage() {
  graphics::Display display = fullScreen();
  graphics::Layer play = layer(320, 200, 16, 30);
  for (int row = 10; row < 190; row += 9) {
    play.rowColors.push_back(
        {row, 3, static_cast<uint16_t>(row * 0x51 & 0xFFF)});
  }
  play.rowColors.push_back({100, 2, play.palette[2]});
  graphics::Layer panel = layer(304, 32, 8, 31);
  panel.top = 210;
  display.layers = {play, panel};
  return display;
}

} // namespace

SCENARIO("Frames that need the copper show what the desktop rasterizer "
         "draws") {
  for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
    GIVEN("A play screen with row colours on colour 3 over a panel in its "
          "own bank") {
      const graphics::Display display = bandedStage();

      THEN("The copper sets the row colours and every pixel matches") {
        Arena arena;
        FrameMemory memory;
        const BuiltFrame frame =
            build(arena, inArena(arena, display), geometry, memory);
        REQUIRE(frame.copper.size() > 1);
        REQUIRE(frame.translations.size() == 1);
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A screen over the top of a picture that starts inside it, both "
          "with row colours") {
      graphics::Display display = fullScreen();
      graphics::Layer lower = layer(320, 100, 16, 32);
      lower.top = 50;
      lower.rowColors.push_back({70, 1, 0x0F0});
      lower.rowColors.push_back({120, 4, 0x00F});
      graphics::Layer upper = layer(320, 100, 16, 33);
      upper.rowColors.push_back({20, 2, 0xF00});
      upper.rowColors.push_back({60, 5, 0xFF0});
      display.layers = {lower, upper};

      THEN("Each row takes the palette of the layer on top") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A picture below an empty band whose row colours come out of "
          "order, one above it and one on its first row") {
      graphics::Display display = fullScreen();
      graphics::Layer picture = layer(320, 150, 16, 34);
      picture.top = 20;
      picture.rowColors.push_back({120, 2, 0x0F0});
      picture.rowColors.push_back({60, 1, 0xF00});
      picture.rowColors.push_back({20, 3, 0x00F});
      picture.rowColors.push_back({5, 1, 0xFF0});
      display.layers = {picture};

      THEN("Each change shows on its own row only") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A masked picture with row colours on colours its mask hides") {
      graphics::Display display = fullScreen();
      graphics::Layer picture = layer(320, 200, 64, 35);
      picture.mask = 0x0F;
      picture.palette.resize(16);
      picture.rowColors.push_back({40, 0x13, 0xF0F});
      picture.rowColors.push_back({40, 2, 0x0FF});
      picture.rowColors.push_back({80, 0x2F, 0x00F});
      display.layers = {picture};

      THEN("Those changes do nothing") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A picture whose pixels go past its palette and a row colour on "
          "one of those values") {
      graphics::Display display = fullScreen();
      graphics::Layer picture = layer(320, 200, 32, 36);
      picture.palette.resize(16);
      picture.rowColors.push_back({30, 20, 0xF80});
      picture.rowColors.push_back({90, 25, 0x08F});
      display.layers = {picture};

      THEN("Those pixels are black except on the changed rows") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A rainbow behind a picture and a panel over it with a row colour "
          "of its own") {
      graphics::Display display = fullScreen();
      graphics::Layer sky = layer(320, 256, 32, 37);
      for (int row = 0; row < 256; ++row) {
        sky.rowColors.push_back(
            {row, 0, static_cast<uint16_t>(row * 0x13 & 0xFFF)});
      }
      graphics::Layer panel = layer(320, 40, 8, 38);
      panel.top = 210;
      panel.rowColors.push_back({220, 2, 0xF0F});
      display.layers = {sky, panel};

      THEN("The rainbow comes from line phrases and the panel from the "
           "copper") {
        Arena arena;
        FrameMemory memory;
        const BuiltFrame frame =
            build(arena, inArena(arena, display), geometry, memory);
        REQUIRE(frame.lineLayer == 0);
        REQUIRE(frame.copper.size() > 1);
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }
  }
}

SCENARIO("Copper frames follow every change between builds") {
  GIVEN("The banded play screen built twice into the same slot") {
    const Geometry geometry = palGeometry();
    Arena arena;
    ArenaBuffers buffers(arena);
    const FrameMemory memory = frameMemory(arena, buffers);
    graphics::Display shown = inArena(arena, bandedStage());
    BuiltFrame first;
    buildFrame(shown, geometry, memory, nullptr, 0, first);
    BuiltFrame second;
    buildFrame(shown, geometry, memory, nullptr, 0, second);

    THEN("The second build gives the same copper list") {
      REQUIRE(second.copper == first.copper);
      REQUIRE(second.clut == first.clut);
    }

    THEN("New row colours, palettes, masks and places each show") {
      const auto check = [&](const graphics::Display &next) {
        BuiltFrame frame;
        buildFrame(next, geometry, memory, nullptr, 0, frame);
        for (const Translation &translation : frame.translations) {
          translateOnCpu(translation);
        }
        REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
      };
      graphics::Display next = shown;
      next.layers[0].rowColors.push_back({150, 3, 0x0F0});
      check(next);
      next.layers[0].palette[3] = 0x0AF;
      check(next);
      next.layers[1].mask = 0x03;
      check(next);
      next.layers[1].top += 4;
      check(next);
      next.layers.pop_back();
      check(next);
    }
  }
}

namespace {

class FailingBuffers : public TranslationBuffers {
public:
  uint8_t *buffer(const uint8_t *, std::size_t) override { return nullptr; }
};

class CopperBan {
public:
  CopperBan() { allowCopper(false); }
  ~CopperBan() { allowCopper(true); }
  CopperBan(const CopperBan &) = delete;
  CopperBan &operator=(const CopperBan &) = delete;
};

graphics::Display plainStage() {
  graphics::Display display;
  display.width = 304;
  display.height = 255;
  display.displayHeight = 255;
  display.layers.push_back(layer(320, 222, 16, 51));
  display.layers.back().columns = 304;
  graphics::Layer panel = layer(304, 32, 8, 52);
  panel.top = 223;
  display.layers.push_back(panel);
  return display;
}

graphics::Display rainbowPicture() {
  graphics::Display display = fullScreen();
  graphics::Layer picture = layer(640, 256, 32, 53);
  picture.wrap = true;
  picture.columns = 320;
  for (int row = 0; row < 256; ++row) {
    picture.rowColors.push_back(
        {row, 0, static_cast<uint16_t>(row * 0x17 & 0xFFF)});
  }
  display.layers = {picture};
  return display;
}

} // namespace

SCENARIO("Unusual layers still show what the desktop rasterizer draws") {
  for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
    GIVEN("A picture shown three rows to each of its rows") {
      graphics::Display display = fullScreen();
      graphics::Layer stretched = layer(320, 60, 16, 40);
      stretched.top = 12;
      stretched.rows = 180;
      stretched.repeat = 3;
      display.layers = {stretched};

      THEN("Every pixel matches") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A strip of 40000 rows shown from row 35000") {
      graphics::Display display;
      display.width = 32;
      display.height = 200;
      display.displayHeight = 200;
      graphics::Layer strip = layer(32, 40000, 16, 41);
      strip.sourceY = 35000;
      strip.rows = 200;
      display.layers = {strip};

      THEN("The rows past 32767 are found as on the desktop") {
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A picture using all 256 colours under a band of a colour it "
          "lacks") {
      graphics::Display display = fullScreen();
      graphics::Layer picture = layer(320, 256, 256, 47);
      for (std::size_t value = 0; value < picture.palette.size(); ++value) {
        picture.palette[value] = static_cast<uint16_t>(value);
      }
      display.layers = {picture, graphics::solidLayer(0xF00, 100, 20, 320)};

      THEN("The copper lends the band a colour on its rows") {
        Arena arena;
        FrameMemory memory;
        const BuiltFrame frame =
            build(arena, inArena(arena, display), geometry, memory);
        REQUIRE(frame.copper.size() > 1);
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    GIVEN("A rainbow picture and frame memory without line phrases") {
      const graphics::Display display = rainbowPicture();

      THEN("The copper sets colour 0 on every row instead") {
        Arena arena;
        ArenaBuffers buffers(arena);
        FrameMemory memory = frameMemory(arena, buffers);
        memory.linePhrases = nullptr;
        memory.lineCapacity = 0;
        const graphics::Display shown = inArena(arena, display);
        BuiltFrame frame;
        buildFrame(shown, geometry, memory, nullptr, 0, frame);
        REQUIRE(frame.lineLayer == -1);
        REQUIRE(frame.copper.size() > 1);
        REQUIRE(wrongPixels(arena, frame, memory, shown, geometry) == 0);
      }
    }
  }
}

SCENARIO("A layer outside the display gets no object") {
  GIVEN("A panel placed below and to the right of the display") {
    graphics::Display display = fullScreen();
    display.layers.push_back(layer(320, 256, 16, 43));
    graphics::Layer away = layer(64, 32, 8, 44);
    away.top = 300;
    away.left = 400;
    display.layers.push_back(away);

    THEN("Its area is empty, it gets no object and the rest matches") {
      for (const Geometry &geometry : {palGeometry(), ntscGeometry()}) {
        const Placement placement = placeDisplay(display, geometry);
        const LayerArea area =
            visibleArea(display, display.layers[1], placement, geometry);
        REQUIRE(area.lastRow == area.firstRow);
        REQUIRE(area.lastColumn == area.firstColumn);
        Arena arena;
        FrameMemory memory;
        const BuiltFrame frame =
            build(arena, inArena(arena, display), geometry, memory);
        REQUIRE(frame.objects[0] != -1);
        REQUIRE(frame.objects[1] == -1);
        REQUIRE(mismatches(display, geometry) == 0);
      }
    }

    THEN("Fading the colours only changes the colour table") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display shown = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      const std::vector<uint64_t> phrases = frame.phrases;
      graphics::Display faded = shown;
      for (graphics::Layer &fading : faded.layers) {
        for (uint16_t &color : fading.palette) {
          color = static_cast<uint16_t>((color >> 1) & 0x777);
        }
      }
      REQUIRE(recolorFrame(faded, shown, geometry, memory, frame));
      REQUIRE(frame.phrases == phrases);
      REQUIRE(wrongPixels(arena, frame, memory, faded, geometry) == 0);
    }
  }
}

SCENARIO("Sprites the object processor cannot show are left out") {
  GIVEN("A picture carrying a sprite 12 pixels wide and one 16 wide") {
    const Bob odd = bob(12, 10, 4);
    const Bob even = bob(16, 10, 4);
    graphics::Display picture = fullScreen();
    picture.layers = {layer(320, 256, 16, 42)};

    THEN("The narrow one is not drawn and the other is") {
      const Geometry geometry = palGeometry();
      Arena arena;
      const graphics::Display shown =
          inArena(arena, withBobs(picture, {&odd}, {{100, 100}}));
      FrameMemory memory;
      const BuiltFrame frame = build(arena, shown, geometry, memory);
      graphics::Display bare = shown;
      bare.layers.front().sprites.clear();
      REQUIRE(wrongPixels(arena, frame, memory, bare, geometry) == 0);
      REQUIRE(mismatches(withBobs(picture, {&even}, {{100, 100}}), geometry) ==
              0);
    }
  }

  GIVEN("A display with no rows whose layer carries a sprite") {
    const Bob small = bob(16, 12, 1);
    graphics::Display empty;
    empty.width = 320;
    graphics::Layer screen = layer(320, 10, 16, 45);
    screen.rows = 0;
    empty.layers = {screen};

    THEN("It gets its sprite slots but no border masks") {
      Arena arena;
      FrameMemory memory;
      const BuiltFrame frame =
          build(arena, inArena(arena, withBobs(empty, {&small}, {{10, 10}})),
                palGeometry(), memory);
      REQUIRE(frame.spriteSlots.size() == 1);
      REQUIRE(frame.masks.empty());
    }
  }
}

SCENARIO("Overlays are drawn over the frame where they are placed") {
  GIVEN("A picture and an 8 by 4 overlay of 16-bit colours") {
    THEN("The overlay replaces the picture there and nowhere else") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      graphics::Display display = fullScreen();
      display.layers = {layer(320, 256, 16, 46)};
      const graphics::Display shown = inArena(arena, display);
      uint8_t *pixels = arena.allocate(8 * 4 * 2);
      for (int at = 0; at < 32; ++at) {
        const uint16_t color = static_cast<uint16_t>(0x1000 + at * 0x111);
        pixels[at * 2] = static_cast<uint8_t>(color >> 8);
        pixels[at * 2 + 1] = static_cast<uint8_t>(color);
      }
      const Overlay overlay{arena.address(pixels), 8, 4, 30, 40};
      BuiltFrame plain;
      buildFrame(shown, geometry, memory, nullptr, 0, plain);
      const Screen before =
          simulate(arena, plain, memory.liveAddress, geometry);
      BuiltFrame covered;
      buildFrame(shown, geometry, memory, &overlay, 1, covered);
      const Screen after =
          simulate(arena, covered, memory.liveAddress, geometry);
      int covers = 0;
      int wrong = 0;
      for (std::size_t row = 0; row < after.size(); ++row) {
        for (std::size_t column = 0; column < after[row].size(); ++column) {
          const bool inside =
              row >= 40 && row < 44 && column >= 30 && column < 38;
          const uint16_t expected =
              inside ? static_cast<uint16_t>(
                           0x1000 + ((row - 40) * 8 + column - 30) * 0x111)
                     : before[row][column];
          covers += inside && before[row][column] != expected ? 1 : 0;
          wrong += after[row][column] != expected ? 1 : 0;
        }
      }
      REQUIRE(covers == 32);
      REQUIRE(wrong == 0);
    }
  }
}

SCENARIO("Display comparisons look at every layer") {
  GIVEN("A display with row colours and a copy whose equal row colours are "
        "its own") {
    graphics::Display display = fullScreen();
    display.layers = {layer(320, 256, 16, 54)};
    display.layers[0].rowColors = {{1, 2, 0xF00}, {3, 1, 0x0F0}};
    graphics::Display copy = display;
    copy.layers[0].rowColors = {{1, 2, 0xF00}, {3, 1, 0x0F0}};

    THEN("They have the same colours until a row colour differs") {
      REQUIRE_FALSE(
          copy.layers[0].rowColors.shares(display.layers[0].rowColors));
      REQUIRE(sameColors(display, copy));
      REQUIRE(sameLayers(display, copy));
      copy.layers[0].rowColors = {{1, 2, 0xF00}, {3, 1, 0x0F1}};
      REQUIRE_FALSE(sameColors(display, copy));
    }
  }

  GIVEN("Displays with one and two layers") {
    graphics::Display one = fullScreen();
    one.layers = {layer(320, 256, 16, 55)};
    graphics::Display two = one;
    two.layers.push_back(layer(320, 20, 4, 56));

    THEN("Their sprites never compare the same") {
      REQUIRE_FALSE(sameSprites(one, two));
      REQUIRE_FALSE(sameSprites(two, one));
      REQUIRE_FALSE(sameSprites(one, SpriteLists(2)));
      REQUIRE(sameSprites(one, SpriteLists(1)));
    }
  }
}

SCENARIO("The copper can be switched off") {
  GIVEN("A play screen whose row colours need the copper") {
    const graphics::Display display = bandedStage();

    THEN("Without the copper the frame has none, and it is back afterwards") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display shown = inArena(arena, display);
      {
        const CopperBan ban;
        BuiltFrame frame;
        buildFrame(shown, palGeometry(), memory, nullptr, 0, frame);
        REQUIRE(frame.copper == std::vector<uint32_t>{COPPER_END});
      }
      BuiltFrame frame;
      buildFrame(shown, palGeometry(), memory, nullptr, 0, frame);
      REQUIRE(frame.copper.size() > 1);
    }
  }
}

SCENARIO("Colour banks are planned again when buffers run out or colours "
         "change") {
  GIVEN("A stage whose panel has a bank of its own") {
    const Geometry geometry = palGeometry();

    THEN("Built again with no translation buffer left, the panel is drawn "
         "through the copper") {
      Arena arena;
      ArenaBuffers buffers(arena);
      FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, geometry, memory);
      const graphics::Display shown = inArena(arena, plainStage());
      BuiltFrame first;
      buildFrame(shown, geometry, memory, nullptr, 0, first);
      REQUIRE(first.translations.size() == 1);
      FailingBuffers full;
      memory.buffers = &full;
      BuiltFrame second;
      buildFrame(shown, geometry, memory, nullptr, 0, second);
      REQUIRE(second.translations.empty());
      REQUIRE(second.copper.size() > 1);
      REQUIRE(wrongPixels(arena, second, memory, shown, geometry) == 0);
    }

    THEN("When the panel could share the first bank after the plan was "
         "forgotten, recolouring is left to a rebuild") {
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, geometry, memory);
      const graphics::Display built = inArena(arena, plainStage());
      BuiltFrame frame;
      buildFrame(built, geometry, memory, nullptr, 0, frame);
      REQUIRE(frame.translations.size() == 1);
      forgetBanks(arena, geometry, memory);
      graphics::Display next = built;
      std::copy_n(next.layers[0].palette.begin(), next.layers[1].palette.size(),
                  next.layers[1].palette.begin());
      REQUIRE_FALSE(recolorFrame(next, built, geometry, memory, frame));
      buildFrame(next, geometry, memory, nullptr, 0, frame);
      REQUIRE(frame.translations.empty());
      REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
    }
  }

  GIVEN("A 160-colour picture and a panel that only fits the last bank") {
    graphics::Display display = fullScreen();
    display.layers.push_back(layer(320, 200, 160, 57));
    graphics::Layer panel = layer(320, 40, 8, 58);
    panel.top = 210;
    display.layers.push_back(panel);

    THEN("Colours that would move the panel to another bank after the plan "
         "was forgotten are left to a rebuild") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, geometry, memory);
      const graphics::Display built = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(built, geometry, memory, nullptr, 0, frame);
      REQUIRE(frame.translations.size() == 1);
      REQUIRE(frame.translations[0].flip == 0xC0C0C0C0u);
      forgetBanks(arena, geometry, memory);
      graphics::Display next = built;
      std::copy_n(next.layers[1].palette.begin(), next.layers[1].palette.size(),
                  next.layers[0].palette.begin() + 0x80);
      REQUIRE_FALSE(recolorFrame(next, built, geometry, memory, frame));
      buildFrame(next, geometry, memory, nullptr, 0, frame);
      for (const Translation &translation : frame.translations) {
        translateOnCpu(translation);
      }
      REQUIRE(frame.translations[0].flip == 0x80808080u);
      REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
    }
  }

  GIVEN("A picture and a panel whose eight colours are the picture's first "
        "eight") {
    graphics::Display display = fullScreen();
    display.layers.push_back(layer(320, 200, 16, 48));
    graphics::Layer panel = layer(320, 40, 8, 49);
    panel.top = 210;
    std::copy_n(display.layers[0].palette.begin(), 8, panel.palette.begin());
    display.layers.push_back(panel);

    THEN("A new colour the panel does not use keeps the plan, so the other "
         "frame of the pair can follow") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      forgetBanks(arena, geometry, memory);
      const graphics::Display shown = inArena(arena, display);
      BuiltFrame frame;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      REQUIRE(frame.translations.empty());
      BuiltFrame twin = frame;
      graphics::Display next = shown;
      next.layers[0].palette[12] = 0x0F0;
      REQUIRE(recolorFrame(next, shown, geometry, memory, frame));
      REQUIRE(frame.plan == twin.plan);
      REQUIRE(recolorPalettes(next, twin));
      REQUIRE(twin.clut == frame.clut);
      REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
      REQUIRE(freshClut(arena, next, geometry, memory) == frame.clut);
    }
  }
}

SCENARIO("Frames that cannot be patched are left to a rebuild") {
  GIVEN("A stage built for PAL") {
    const Geometry geometry = palGeometry();
    Arena arena;
    ArenaBuffers buffers(arena);
    const FrameMemory memory = frameMemory(arena, buffers);
    forgetBanks(arena, geometry, memory);
    const graphics::Display built = inArena(arena, plainStage());
    BuiltFrame frame;
    buildFrame(built, geometry, memory, nullptr, 0, frame);
    for (const Translation &translation : frame.translations) {
      translateOnCpu(translation);
    }
    REQUIRE(frame.translations.size() == 1);

    THEN("Another border, palette size or screen is refused") {
      graphics::Display border = built;
      border.border = 0x123;
      BuiltFrame copy = frame;
      REQUIRE_FALSE(scrollFrame(border, built, geometry, memory, copy));
      graphics::Display longer = built;
      longer.layers[1].palette.push_back(0x777);
      copy = frame;
      REQUIRE_FALSE(scrollFrame(longer, built, geometry, memory, copy));
      graphics::Display moved = built;
      moved.layers[0].sourceX += 2;
      copy = frame;
      REQUIRE_FALSE(scrollFrame(moved, built, ntscGeometry(), memory, copy));
    }

    THEN("A panel scrolled out of its pixels is refused") {
      graphics::Display gone = built;
      gone.layers[1].sourceX = 304;
      BuiltFrame copy = frame;
      REQUIRE_FALSE(scrollFrame(gone, built, geometry, memory, copy));
    }

    THEN("A panel flipped to new pixels with no buffer for them is refused") {
      const graphics::Layer otherPanel = layer(304, 32, 8, 59);
      graphics::Display flipped = built;
      flipped.layers[1].pixels = arenaCopy(arena, otherPanel.pixels, 304 * 32);
      FrameMemory withoutBuffers = memory;
      withoutBuffers.buffers = nullptr;
      BuiltFrame copy = frame;
      REQUIRE_FALSE(
          scrollFrame(flipped, built, geometry, withoutBuffers, copy));
      FailingBuffers full;
      FrameMemory fullBuffers = memory;
      fullBuffers.buffers = &full;
      copy = frame;
      REQUIRE_FALSE(scrollFrame(flipped, built, geometry, fullBuffers, copy));
      copy = frame;
      REQUIRE(scrollFrame(flipped, built, geometry, memory, copy));
      for (const Translation &translation : copy.translations) {
        translateOnCpu(translation);
      }
      REQUIRE(wrongPixels(arena, copy, memory, flipped, geometry) == 0);
    }
  }

  GIVEN("A play screen whose row colours need the copper") {
    const Geometry geometry = palGeometry();
    Arena arena;
    ArenaBuffers buffers(arena);
    const FrameMemory memory = frameMemory(arena, buffers);
    const graphics::Display built = inArena(arena, bandedStage());
    BuiltFrame frame;
    buildFrame(built, geometry, memory, nullptr, 0, frame);

    THEN("Scrolling it is refused") {
      graphics::Display next = built;
      next.layers[0].sourceX += 1;
      REQUIRE_FALSE(scrollFrame(next, built, geometry, memory, frame));
    }
  }

  GIVEN("A rainbow picture with its colours in line phrases") {
    const Geometry geometry = palGeometry();
    Arena arena;
    ArenaBuffers buffers(arena);
    const FrameMemory memory = frameMemory(arena, buffers);
    const graphics::Display built = inArena(arena, rainbowPicture());
    BuiltFrame frame;
    buildFrame(built, geometry, memory, nullptr, 0, frame);
    REQUIRE(frame.lineLayer == 0);

    THEN("Scrolling it into memory without line phrases is refused") {
      graphics::Display next = built;
      next.layers[0].sourceX += 3;
      FrameMemory withoutLines = memory;
      withoutLines.linePhrases = nullptr;
      REQUIRE_FALSE(scrollFrame(next, built, geometry, withoutLines, frame));
    }
  }

  GIVEN("A picture doubled in height carrying one sprite") {
    const Bob small = bob(16, 12, 1);
    graphics::Display display = fullScreen();
    graphics::Layer doubled = layer(320, 128, 16, 60);
    doubled.rows = 256;
    doubled.repeat = 2;
    display.layers = {doubled};

    THEN("A second sprite, which needs a scaled object, is left to a "
         "rebuild") {
      const Geometry geometry = palGeometry();
      Arena arena;
      ArenaBuffers buffers(arena);
      const FrameMemory memory = frameMemory(arena, buffers);
      const graphics::Display shown =
          inArena(arena, withBobs(display, {&small}, {{40, 30}}));
      BuiltFrame frame;
      buildFrame(shown, geometry, memory, nullptr, 0, frame);
      graphics::Display next = inArena(
          arena, withBobs(display, {&small, &small}, {{40, 30}, {80, 50}}));
      next.layers.front().pixels = shown.layers.front().pixels;
      BuiltFrame copy = frame;
      REQUIRE_FALSE(scrollFrame(next, shown, geometry, memory, copy));
      copy = frame;
      REQUIRE_FALSE(moveSprites(next, shown, geometry, memory, copy));
      buildFrame(next, geometry, memory, nullptr, 0, frame);
      REQUIRE(wrongPixels(arena, frame, memory, next, geometry) == 0);
    }
  }
}
