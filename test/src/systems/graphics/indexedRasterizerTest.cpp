#include "../../../../src/systems/graphics/IndexedRasterizer.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace openfranko::src::systems::graphics;

namespace {

const std::vector<uint16_t> LEVEL_COLORS = {
    0x555, 0xAAA, 0x666, 0xFAA, 0x083, 0x902, 0xB95, 0x760,
    0x063, 0x000, 0x520, 0x17A, 0x09E, 0x4DF, 0x777, 0xDDD};
const std::array<int, 4> SHAKE = {0, -8, -4, 0};
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

void scroll(Random &random, const Layer &layer, std::vector<uint8_t> &source) {
  if (layer.sourceColumns <= SHIFT_STEP) {
    return;
  }
  const bool left = random.below(2) == 0;
  const int first = random.below(layer.sourceRows);
  const int last =
      std::min(layer.sourceRows, first + random.between(1, layer.sourceRows));
  const std::size_t kept =
      static_cast<std::size_t>(layer.sourceColumns - SHIFT_STEP);
  for (int y = first; y < last; ++y) {
    uint8_t *row =
        source.data() + static_cast<std::ptrdiff_t>(y) * layer.stride;
    uint8_t *exposed = left ? row + kept : row;
    if (left) {
      std::memmove(row, row + SHIFT_STEP, kept);
    } else {
      std::memmove(row + SHIFT_STEP, row, kept);
    }
    for (int column = 0; column < SHIFT_STEP; ++column) {
      exposed[column] = static_cast<uint8_t>(random.below(40));
    }
  }
}

void paint(Random &random, const Layer &layer, std::vector<uint8_t> &source) {
  const int left = random.below(layer.sourceColumns);
  const int top = random.below(layer.sourceRows);
  const int right = std::min(layer.sourceColumns, left + random.between(1, 12));
  const int bottom = std::min(layer.sourceRows, top + random.between(1, 12));
  const uint8_t value = static_cast<uint8_t>(random.below(40));
  for (int y = top; y < bottom; ++y) {
    std::fill(source.begin() + y * layer.stride + left,
              source.begin() + y * layer.stride + right, value);
  }
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
  for (const Layer &layer : display.layers) {
    const auto source =
        std::find_if(sources.begin(), sources.end(),
                     [&layer](const std::vector<uint8_t> &pixels) {
                       return pixels.data() == layer.pixels;
                     });
    if (source == sources.end()) {
      continue;
    }
    if (random.below(3) == 0) {
      scroll(random, layer, *source);
    }
    if (random.below(2) == 0) {
      paint(random, layer, *source);
    }
  }
  if (display.layers.empty() || random.below(3) != 0) {
    return;
  }
  Layer &layer = display.layers[static_cast<std::size_t>(
      random.below(static_cast<int>(display.layers.size())))];
  switch (random.below(5)) {
  case 0:
    layer.sourceX += random.between(-2, 2);
    break;
  case 4:
    layer.sourceY += random.between(-3, 3);
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

void applyChanges(const IndexedFrame &frame, std::vector<uint8_t> &screen) {
  screen.resize(frame.pixels.size());
  const std::vector<uint8_t> before = screen;
  for (int row = 0; row < frame.height; ++row) {
    const RowChange &change = frame.changes[static_cast<std::size_t>(row)];
    const std::ptrdiff_t offset =
        static_cast<std::ptrdiff_t>(row) * frame.width;
    uint8_t *line = screen.data() + offset;
    if (change.from != NO_ROW) {
      const auto from = before.begin() +
                        static_cast<std::ptrdiff_t>(change.from) * frame.width;
      std::copy(from, from + frame.width, line);
    }
    const std::size_t kept =
        static_cast<std::size_t>(frame.width - std::abs(change.shift));
    if (change.shift < 0) {
      std::memmove(line, line - change.shift, kept);
    } else if (change.shift > 0) {
      std::memmove(line + change.shift, line, kept);
    }
    for (const Span &span : change.spans) {
      std::copy(frame.pixels.begin() + offset + span.first,
                frame.pixels.begin() + offset + span.last, line + span.first);
    }
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
  GIVEN("Random displays that scroll and change a little from frame to frame") {
    Random random;

    THEN("Each frame matches rasterize and its changes update the last one") {
      for (int round = 0; round < 500; ++round) {
        std::vector<std::vector<uint8_t>> sources;
        Display display = randomDisplay(random, sources);
        IndexedRasterizer rasterizer;
        IndexedFrame frame;
        std::vector<uint8_t> screen;
        for (int step = 0; step < 6; ++step) {
          rasterizer.rasterize(display, frame);
          REQUIRE(colorsOf(frame) == expected(display));
          applyChanges(frame, screen);
          REQUIRE(screen == frame.pixels);
          mutate(random, display, sources);
        }
      }
    }
  }

  GIVEN("A double buffered street that scrolls left under a panel") {
    Random random;
    std::array<std::vector<uint8_t>, 2> buffers;
    for (std::vector<uint8_t> &buffer : buffers) {
      buffer = pattern(64, 40, 16);
    }
    const std::vector<uint8_t> panel = pattern(48, 6, 8);
    Display display = screen(48, 40);
    Layer street = layer(buffers[0], 64, 40, LEVEL_COLORS);
    street.sourceX = 8;
    street.columns = 48;
    street.rows = 34;
    display.layers.push_back(street);
    Layer panelLayer = layer(panel, 48, 6, PANEL_COLORS);
    panelLayer.top = 34;
    display.layers.push_back(panelLayer);
    IndexedRasterizer rasterizer;
    IndexedFrame frame;
    std::vector<uint8_t> screenPixels;
    int shiftedRows = 0;
    int movedRows = 0;
    for (int step = 0; step < 12; ++step) {
      std::vector<uint8_t> &shown = buffers[static_cast<std::size_t>(step % 2)];
      if (step % 4 == 1) {
        for (std::vector<uint8_t> &buffer : buffers) {
          for (int y = 0; y < 40; ++y) {
            uint8_t *row = buffer.data() + y * 64;
            std::memmove(row, row + SHIFT_STEP, 64 - SHIFT_STEP);
            std::fill(row + 64 - SHIFT_STEP, row + 64,
                      static_cast<uint8_t>(y % 16));
          }
        }
      }
      paint(random, street, shown);
      display.layers[0].pixels = shown.data();
      display.layers[0].sourceY = SHAKE[static_cast<std::size_t>(step % 4)];
      rasterizer.rasterize(display, frame);
      REQUIRE(colorsOf(frame) == expected(display));
      applyChanges(frame, screenPixels);
      REQUIRE(screenPixels == frame.pixels);
      movedRows += static_cast<int>(std::count_if(
          frame.changes.begin(), frame.changes.end(),
          [](const RowChange &change) { return change.from != NO_ROW; }));
      if (step % 4 == 1) {
        shiftedRows += static_cast<int>(
            std::count_if(frame.changes.begin(), frame.changes.end(),
                          [](const RowChange &change) {
                            return change.shift == -SHIFT_STEP;
                          }));
      }
    }

    THEN("Most rows of a scrolled frame are moved rather than redrawn") {
      REQUIRE(shiftedRows > 2 * 34 / 2);
    }

    THEN("A shaken frame moves its rows up or down") {
      REQUIRE(movedRows > 8 * 34 / 2);
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
      REQUIRE(
          std::none_of(frame.changes.begin(), frame.changes.end(), isChanged));
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
