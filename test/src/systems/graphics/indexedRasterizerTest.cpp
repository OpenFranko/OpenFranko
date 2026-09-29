#include "../../../../src/systems/graphics/IndexedRasterizer.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace openfranko::src::systems::graphics;

namespace {

const std::vector<uint16_t> LEVEL_COLORS = {
    0x555, 0xAAA, 0x666, 0xFAA, 0x083, 0x902, 0xB95, 0x760,
    0x063, 0x000, 0x520, 0x17A, 0x09E, 0x4DF, 0x777, 0xDDD};
const std::vector<uint16_t> PANEL_COLORS = {0x555, 0x000, 0xF10, 0x666,
                                            0x888, 0x999, 0xAAA, 0xDDD};

class Random {
public:
  int below(int limit) {
    m_state = m_state * 1103515245u + 12345u;
    return static_cast<int>((m_state >> 8) % static_cast<uint32_t>(limit));
  }

  int between(int low, int high) { return low + below(high - low + 1); }

private:
  uint32_t m_state = 1;
};

std::vector<uint32_t> expected(const Display &display) {
  std::vector<uint32_t> argb;
  rasterize(display, argb);
  return argb;
}

std::vector<uint32_t> colorsOf(const IndexedFrame &frame) {
  std::vector<uint32_t> argb;
  for (const uint8_t pixel : frame.pixels) {
    argb.push_back(toArgb(frame.palette[pixel]));
  }
  return argb;
}

std::vector<uint32_t> shown(const Display &display) {
  IndexedRasterizer rasterizer;
  IndexedFrame frame;
  rasterizer.rasterize(display, frame);
  return colorsOf(frame);
}

Display screen(int width, int height, uint16_t border = 0x555) {
  Display display;
  display.width = width;
  display.height = height;
  display.displayHeight = height;
  display.border = border;
  return display;
}

Layer layer(const std::vector<uint8_t> &pixels, int columns, int rows,
            std::vector<uint16_t> palette) {
  Layer result;
  result.pixels = pixels.data();
  result.stride = columns;
  result.sourceColumns = columns;
  result.sourceRows = rows;
  result.columns = columns;
  result.rows = rows;
  result.palette = std::move(palette);
  return result;
}

std::vector<uint8_t> pattern(int columns, int rows, int colors) {
  std::vector<uint8_t> pixels;
  for (int row = 0; row < rows; ++row) {
    for (int column = 0; column < columns; ++column) {
      pixels.push_back(static_cast<uint8_t>((row * 7 + column * 3) % colors));
    }
  }
  return pixels;
}

std::vector<uint16_t> randomPalette(Random &random) {
  std::vector<uint16_t> palette(
      static_cast<std::size_t>(random.between(1, 24)));
  for (uint16_t &color : palette) {
    color = static_cast<uint16_t>(random.below(0x1000));
  }
  return palette;
}

Display randomDisplay(Random &random,
                      std::vector<std::vector<uint8_t>> &sources) {
  Display display = screen(random.between(1, 40), random.between(1, 30),
                           static_cast<uint16_t>(random.below(0x1000)));
  const std::array<uint8_t, 6> masks = {0xFF, 0x0F, 0x1F, 0x07, 0x05, 0x3F};
  const int layers = random.between(0, 4);
  for (int index = 0; index < layers; ++index) {
    Layer added;
    added.palette = randomPalette(random);
    added.mask = masks[static_cast<std::size_t>(random.below(6))];
    added.left = random.between(-10, 30);
    added.top = random.between(-10, 30);
    added.columns = random.between(1, 50);
    added.rows = random.between(1, 40);
    if (random.below(4) != 0) {
      added.sourceColumns = random.between(1, 50);
      added.sourceRows = random.between(1, 40);
      added.stride = added.sourceColumns + random.below(5);
      added.sourceX = random.between(-10, 50);
      added.sourceY = random.between(-5, 40);
      added.sourceStep = random.between(1, 3);
      added.repeat = random.between(1, 3);
      added.wrap = random.below(2) == 0;
      const int values = random.below(3) == 0 ? 256 : 40;
      sources.emplace_back(static_cast<std::size_t>(added.stride) *
                           added.sourceRows);
      for (uint8_t &pixel : sources.back()) {
        pixel = static_cast<uint8_t>(random.below(values));
      }
      added.pixels = sources.back().data();
    }
    const int changes = random.below(3) == 0 ? random.between(1, 6) : 0;
    for (int change = 0; change < changes; ++change) {
      added.rowColors.push_back({random.between(-2, 35),
                                 static_cast<uint8_t>(random.below(42)),
                                 static_cast<uint16_t>(random.below(0x1000))});
    }
    display.layers.push_back(std::move(added));
  }
  return display;
}

void mutate(Random &random, Display &display,
            std::vector<std::vector<uint8_t>> &sources) {
  for (std::vector<uint8_t> &source : sources) {
    const int changes = random.below(4);
    for (int change = 0; change < changes; ++change) {
      source[static_cast<std::size_t>(
          random.below(static_cast<int>(source.size())))] =
          static_cast<uint8_t>(random.below(40));
    }
  }
  if (display.layers.empty() || random.below(3) != 0) {
    return;
  }
  Layer &layer = display.layers[static_cast<std::size_t>(
      random.below(static_cast<int>(display.layers.size())))];
  switch (random.below(4)) {
  case 0:
    layer.sourceX += random.between(-2, 2);
    break;
  case 1:
    layer.palette[0] = static_cast<uint16_t>(random.below(0x1000));
    break;
  case 2:
    for (RowColor &change : layer.rowColors) {
      change.color = static_cast<uint16_t>(random.below(0x1000));
    }
    break;
  default:
    display.border = static_cast<uint16_t>(random.below(0x1000));
    break;
  }
}

} // namespace

SCENARIO("IndexedRasterizer shows what rasterize shows") {
  GIVEN("A level display with the play screen, the panel and a black band") {
    const std::vector<uint8_t> play = pattern(40, 30, 16);
    const std::vector<uint8_t> panel = pattern(40, 6, 8);
    Display display = screen(32, 30);
    Layer playLayer = layer(play, 40, 30, LEVEL_COLORS);
    playLayer.sourceX = 5;
    playLayer.columns = 32;
    display.layers.push_back(playLayer);
    Layer panelLayer = layer(panel, 40, 6, PANEL_COLORS);
    panelLayer.top = 24;
    panelLayer.columns = 32;
    display.layers.push_back(panelLayer);
    display.layers.push_back(solidLayer(0x000, 0, 2, 32));

    THEN("Every pixel has the same colour") {
      REQUIRE(shown(display) == expected(display));
    }
  }

  GIVEN("A play screen with pixel values past its 16 colours") {
    std::vector<uint8_t> play = pattern(8, 4, 16);
    play[5] = 200;
    play[17] = 16;
    Display display = screen(8, 4);
    display.layers.push_back(layer(play, 8, 4, LEVEL_COLORS));

    THEN("Those pixels are black as in rasterize") {
      REQUIRE(shown(display) == expected(display));
    }
  }

  GIVEN("A layer with copper colour changes on some rows") {
    const std::vector<uint8_t> pixels = pattern(6, 5, 4);
    Display display = screen(6, 5);
    Layer recolored = layer(pixels, 6, 5, {0x000, 0x111, 0x222, 0x333});
    recolored.rowColors = {{1, 0, 0xF00}, {3, 0, 0x0F0}, {3, 2, 0x00F}};
    display.layers.push_back(recolored);

    THEN("Each row shows its own colours") {
      REQUIRE(shown(display) == expected(display));
    }
  }

  GIVEN("Several thousand random displays") {
    Random random;

    THEN("Each one matches rasterize") {
      for (int round = 0; round < 3000; ++round) {
        std::vector<std::vector<uint8_t>> sources;
        const Display display = randomDisplay(random, sources);
        REQUIRE(shown(display) == expected(display));
      }
    }
  }
}

SCENARIO("IndexedRasterizer redraws only what changed since the last frame") {
  GIVEN("Random displays that change a little from frame to frame") {
    Random random;

    THEN("Each frame matches rasterize and marks every row it changed") {
      for (int round = 0; round < 500; ++round) {
        std::vector<std::vector<uint8_t>> sources;
        Display display = randomDisplay(random, sources);
        IndexedRasterizer rasterizer;
        IndexedFrame frame;
        std::vector<uint8_t> before;
        for (int step = 0; step < 6; ++step) {
          rasterizer.rasterize(display, frame);
          REQUIRE(colorsOf(frame) == expected(display));
          for (int row = 0;
               before.size() == frame.pixels.size() && row < frame.height;
               ++row) {
            const auto offset = static_cast<std::ptrdiff_t>(row) * frame.width;
            if (!std::equal(frame.pixels.begin() + offset,
                            frame.pixels.begin() + offset + frame.width,
                            before.begin() + offset)) {
              REQUIRE(frame.changedRows[static_cast<std::size_t>(row)]);
            }
          }
          before = frame.pixels;
          mutate(random, display, sources);
        }
      }
    }
  }

  GIVEN("A level display that did not change") {
    const std::vector<uint8_t> play = pattern(40, 30, 16);
    Display display = screen(32, 30);
    Layer playLayer = layer(play, 40, 30, LEVEL_COLORS);
    playLayer.columns = 32;
    display.layers.push_back(playLayer);
    IndexedRasterizer rasterizer;
    IndexedFrame frame;
    rasterizer.rasterize(display, frame);
    rasterizer.rasterize(display, frame);

    THEN("No row is marked as changed") {
      REQUIRE(std::none_of(frame.changedRows.begin(), frame.changedRows.end(),
                           [](bool changed) { return changed; }));
    }
  }
}

SCENARIO("IndexedRasterizer keeps the colour indices of a full palette") {
  GIVEN("A 256-colour screen") {
    std::vector<uint16_t> palette(FRAME_COLORS);
    for (std::size_t index = 0; index < palette.size(); ++index) {
      palette[index] = static_cast<uint16_t>(index * 13);
    }
    const std::vector<uint8_t> pixels = pattern(16, 16, 256);
    Display display = screen(16, 16);
    display.layers.push_back(layer(pixels, 16, 16, palette));
    IndexedRasterizer rasterizer;
    IndexedFrame frame;
    rasterizer.rasterize(display, frame);

    THEN("The pixels and the palette are copied unchanged") {
      REQUIRE(frame.pixels == pixels);
      REQUIRE(std::vector<uint16_t>(frame.palette.begin(),
                                    frame.palette.end()) == palette);
    }
  }
}

SCENARIO("IndexedRasterizer maps a layer without room through its colours") {
  GIVEN("Two 256-colour layers with the same colours in another order") {
    std::vector<uint16_t> palette(FRAME_COLORS);
    std::vector<uint16_t> reversed(FRAME_COLORS);
    for (std::size_t index = 0; index < palette.size(); ++index) {
      palette[index] = static_cast<uint16_t>(index * 13);
      reversed[FRAME_COLORS - 1 - index] = palette[index];
    }
    const std::vector<uint8_t> pixels = pattern(8, 8, 256);
    Display display = screen(8, 8);
    display.layers.push_back(layer(pixels, 8, 8, palette));
    Layer second = layer(pixels, 8, 8, reversed);
    second.top = 4;
    display.layers.push_back(second);

    THEN("Both show their own colours") {
      REQUIRE(shown(display) == expected(display));
    }
  }
}

SCENARIO("IndexedRasterizer falls back to the nearest colour past 256") {
  GIVEN("A full 256-colour screen with a border colour it lacks") {
    std::vector<uint16_t> palette(FRAME_COLORS);
    for (std::size_t index = 0; index < palette.size(); ++index) {
      palette[index] = static_cast<uint16_t>(index << 4);
    }
    const std::vector<uint8_t> pixels = pattern(4, 4, 256);
    Display display = screen(6, 4, 0x0F1);
    Layer drawn = layer(pixels, 4, 4, palette);
    display.layers.push_back(drawn);
    IndexedRasterizer rasterizer;
    IndexedFrame frame;
    rasterizer.rasterize(display, frame);

    THEN("The border uses the closest colour in the palette") {
      REQUIRE(frame.palette[frame.pixels[5]] == 0x0F0);
    }
  }
}
