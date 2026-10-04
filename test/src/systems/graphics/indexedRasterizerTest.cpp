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

void changeSources(Random &random, const Display &display,
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
}

void changeLayout(Random &random, Display &display) {
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

void mutate(Random &random, Display &display,
            std::vector<std::vector<uint8_t>> &sources) {
  changeSources(random, display, sources);
  changeLayout(random, display);
}

const uint8_t *spriteImage(Random &random, int width, int height,
                           std::vector<std::vector<uint8_t>> &images) {
  const int values = random.below(3) == 0 ? 256 : 40;
  images.emplace_back(static_cast<std::size_t>(width * height));
  for (uint8_t &pixel : images.back()) {
    pixel =
        random.below(3) == 0 ? 0 : static_cast<uint8_t>(random.below(values));
  }
  return images.back().data();
}

Sprite randomSprite(Random &random, const Layer &layer,
                    std::vector<std::vector<uint8_t>> &images) {
  Sprite sprite;
  sprite.width = static_cast<int16_t>(random.between(1, 12));
  sprite.height = static_cast<int16_t>(random.between(1, 10));
  sprite.pixels = spriteImage(random, sprite.width, sprite.height, images);
  sprite.left = random.between(-6, std::max(layer.sourceColumns, 1) + 2);
  sprite.top = random.between(-6, std::max(layer.sourceRows, 1) + 2);
  return sprite;
}

void addSprites(Random &random, Display &display,
                std::vector<std::vector<uint8_t>> &images) {
  for (Layer &layer : display.layers) {
    const int count = random.below(3) == 0 ? 0 : random.between(1, 4);
    for (int at = 0; at < count; ++at) {
      layer.sprites.push_back(randomSprite(random, layer, images));
    }
    layer.carriesSprites = !layer.sprites.empty();
  }
}

void changeSprites(Random &random, Display &display,
                   std::vector<std::vector<uint8_t>> &images) {
  for (Layer &layer : display.layers) {
    std::vector<Sprite> &sprites = layer.sprites;
    const int count = static_cast<int>(sprites.size());
    Sprite *picked =
        count > 0 ? &sprites[static_cast<std::size_t>(random.below(count))]
                  : nullptr;
    switch (random.below(7)) {
    case 0:
      if (picked) {
        picked->left += random.between(-3, 3);
        picked->top += random.between(-2, 2);
      }
      break;
    case 1:
      if (count < 6) {
        sprites.push_back(randomSprite(random, layer, images));
      }
      break;
    case 2:
      if (picked) {
        sprites.erase(sprites.begin() + (picked - sprites.data()));
      }
      break;
    case 3:
      if (count > 1) {
        std::swap(*picked,
                  sprites[static_cast<std::size_t>(random.below(count))]);
      }
      break;
    case 4:
      if (picked) {
        picked->pixels =
            spriteImage(random, picked->width, picked->height, images);
      }
      break;
    default:
      break;
    }
    layer.carriesSprites = !sprites.empty();
  }
}

int g_scans = 0;

std::size_t countedLeading(const uint8_t *left, const uint8_t *right,
                           std::size_t count) {
  ++g_scans;
  return leadingBytes(left, right, count);
}

std::size_t countedTrailing(const uint8_t *left, const uint8_t *right,
                            std::size_t count) {
  ++g_scans;
  return trailingBytes(left, right, count);
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

  GIVEN("Several thousand random displays with sprites on their layers") {
    Random random;

    THEN("Each one matches rasterize") {
      for (int round = 0; round < 3000; ++round) {
        std::vector<std::vector<uint8_t>> sources;
        std::vector<std::vector<uint8_t>> images;
        Display display = randomDisplay(random, sources);
        addSprites(random, display, images);
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

  GIVEN("Random displays whose sprites move, change, come and go") {
    Random random;

    THEN("Each frame matches rasterize and its changes update the last one") {
      uint32_t revision = 0;
      for (int round = 0; round < 1500; ++round) {
        std::vector<std::vector<uint8_t>> sources;
        std::vector<std::vector<uint8_t>> images;
        Display display = randomDisplay(random, sources);
        addSprites(random, display, images);
        std::vector<uint32_t> revisions(sources.size());
        for (uint32_t &value : revisions) {
          value = random.below(3) == 0 ? 0 : ++revision;
        }
        IndexedRasterizer rasterizer;
        IndexedFrame frame;
        std::vector<uint8_t> screen;
        for (int step = 0; step < 8; ++step) {
          for (Layer &shown : display.layers) {
            for (std::size_t at = 0; at < sources.size(); ++at) {
              if (sources[at].data() == shown.pixels) {
                shown.revision = revisions[at];
              }
            }
          }
          rasterizer.rasterize(display, frame);
          REQUIRE(colorsOf(frame) == expected(display));
          applyChanges(frame, screen);
          REQUIRE(screen == frame.pixels);
          const std::vector<std::vector<uint8_t>> before = sources;
          if (random.below(2) == 0) {
            changeSources(random, display, sources);
          }
          changeLayout(random, display);
          changeSprites(random, display, images);
          for (std::size_t at = 0; at < sources.size(); ++at) {
            if (revisions[at] != 0 && sources[at] != before[at]) {
              revisions[at] = ++revision;
            }
          }
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

  GIVEN("A screen narrower than its picture with a sprite on its rows") {
    std::vector<uint8_t> play = pattern(40, 8, 16);
    const std::vector<uint8_t> image = pattern(8, 4, 15);
    Display display = screen(32, 8);
    Layer playLayer = layer(play, 40, 8, LEVEL_COLORS);
    playLayer.columns = 32;
    playLayer.carriesSprites = true;
    playLayer.sprites.push_back({image.data(), 8, 4, 4, 2});
    display.layers.push_back(playLayer);
    IndexedRasterizer rasterizer;
    IndexedFrame frame;
    std::vector<uint8_t> shownPixels;
    rasterizer.rasterize(display, frame);
    applyChanges(frame, shownPixels);

    WHEN("The sprite moves while a hidden column of its rows changes") {
      play[3 * 40 + 36] ^= 1;
      display.layers[0].sprites[0].left = 9;
      rasterizer.rasterize(display, frame);
      applyChanges(frame, shownPixels);

      THEN("The sprite is shown where it moved to") {
        REQUIRE(colorsOf(frame) == expected(display));
        REQUIRE(shownPixels == frame.pixels);
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
      REQUIRE(
          std::none_of(frame.changes.begin(), frame.changes.end(), isChanged));
    }
  }
}

SCENARIO("IndexedRasterizer pans a wide wrapped picture by shifting rows") {
  GIVEN("A double buffered graveyard with copper rows, a pinned title and a "
        "hand") {
    constexpr int SOURCE_WIDTH = 100;
    constexpr int SOURCE_HEIGHT = 20;
    constexpr int WIDTH = 37;
    constexpr int TITLE_X = 9;
    std::vector<uint8_t> picture = pattern(SOURCE_WIDTH, SOURCE_HEIGHT, 32);
    for (std::size_t at = 0; at < picture.size(); at += 3) {
      picture[at] = 0;
    }
    std::array<std::vector<uint8_t>, 2> buffers{picture, picture};
    std::vector<uint16_t> colors;
    for (int color = 0; color < 32; ++color) {
      colors.push_back(static_cast<uint16_t>(0x111 * (color % 16) + color));
    }
    Display display = screen(WIDTH, SOURCE_HEIGHT);
    Layer graveyard = layer(buffers[0], SOURCE_WIDTH, SOURCE_HEIGHT, colors);
    graveyard.columns = WIDTH;
    graveyard.wrap = true;
    for (int row = 0; row < 16; ++row) {
      graveyard.rowColors.push_back(
          {row, 0, static_cast<uint16_t>(0x00F + 0x100 * (row % 6))});
    }
    display.layers.push_back(graveyard);
    const auto draw = [&](std::vector<uint8_t> &buffer, int offset) {
      buffer = picture;
      for (int y = 4; y < 8; ++y) {
        std::fill_n(buffer.begin() + y * SOURCE_WIDTH + std::min(offset, 60) +
                        TITLE_X,
                    12, static_cast<uint8_t>(5 + y));
      }
      for (int y = 12; y < 16; ++y) {
        std::fill_n(buffer.begin() + y * SOURCE_WIDTH + 70, 6,
                    static_cast<uint8_t>(offset % 3 + 20));
      }
      for (int y = 2; y < 4; ++y) {
        std::fill_n(buffer.begin() + y * SOURCE_WIDTH + 2, 4,
                    static_cast<uint8_t>(offset % 5 + 24));
      }
    };
    IndexedRasterizer rasterizer;
    IndexedFrame frame;
    std::vector<uint8_t> shownPixels;
    int offset = 0;
    int pannedFrames = 0;
    int shiftedRows = 0;
    int drawnPixels = 0;
    int stillChanges = 0;
    for (int step = 0; step < 60; ++step) {
      const bool pans = step < 50;
      const bool fades = step >= 54;
      const int moved = pans ? 1 + step % 2 : 0;
      offset += moved;
      std::vector<uint8_t> &buffer =
          buffers[static_cast<std::size_t>(step % 2)];
      draw(buffer, offset);
      Layer &shown = display.layers[0];
      shown.pixels = buffer.data();
      shown.sourceX = offset;
      if (fades) {
        for (uint16_t &color : shown.palette) {
          color = static_cast<uint16_t>(color & 0xEEE) >> 1;
        }
      }
      rasterizer.rasterize(display, frame);
      REQUIRE(colorsOf(frame) == expected(display));
      applyChanges(frame, shownPixels);
      REQUIRE(shownPixels == frame.pixels);
      if (pans && step > 0) {
        ++pannedFrames;
        for (const RowChange &change : frame.changes) {
          shiftedRows += change.shift == -moved ? 1 : 0;
          for (const Span &span : change.spans) {
            drawnPixels += std::max(0, span.last - span.first);
          }
        }
      }
      if (!pans) {
        stillChanges += static_cast<int>(std::count_if(
            frame.changes.begin(), frame.changes.end(), isChanged));
      }
    }

    THEN("Panned frames shift every row and draw only a few columns") {
      REQUIRE(shiftedRows == pannedFrames * SOURCE_HEIGHT);
      REQUIRE(drawnPixels < pannedFrames * SOURCE_HEIGHT * WIDTH / 3);
    }

    THEN("Frames that only change colours or nothing redraw no row") {
      REQUIRE(stillChanges == 0);
    }
  }
}

SCENARIO("IndexedRasterizer draws sprites over a picture that did not change") {
  GIVEN("Two copies of a wrapped graveyard with copper rows sharing a "
        "revision, a title sprite, an animated hand and a sprite past the "
        "wrap") {
    constexpr int SOURCE_WIDTH = 100;
    constexpr int SOURCE_HEIGHT = 20;
    constexpr int WIDTH = 37;
    constexpr int TITLE_X = 9;
    std::vector<uint8_t> picture = pattern(SOURCE_WIDTH, SOURCE_HEIGHT, 32);
    for (std::size_t at = 0; at < picture.size(); at += 3) {
      picture[at] = 0;
    }
    const std::array<std::vector<uint8_t>, 2> buffers{picture, picture};
    std::vector<uint16_t> colors;
    for (int color = 0; color < 32; ++color) {
      colors.push_back(static_cast<uint16_t>(0x111 * (color % 16) + color));
    }
    Display display = screen(WIDTH, SOURCE_HEIGHT);
    Layer graveyard = layer(buffers[0], SOURCE_WIDTH, SOURCE_HEIGHT, colors);
    graveyard.columns = WIDTH;
    graveyard.wrap = true;
    graveyard.revision = 7;
    graveyard.carriesSprites = true;
    for (int row = 0; row < 16; ++row) {
      graveyard.rowColors.push_back(
          {row, 0, static_cast<uint16_t>(0x00F + 0x100 * (row % 6))});
    }
    display.layers.push_back(graveyard);
    std::vector<uint8_t> title = pattern(16, 4, 9);
    std::array<std::vector<uint8_t>, 2> hands{pattern(8, 4, 3),
                                              pattern(8, 4, 5)};
    std::array<std::vector<uint8_t>, 5> wrapped;
    for (std::size_t at = 0; at < wrapped.size(); ++at) {
      wrapped[at].assign(8 * 2, static_cast<uint8_t>(at + 24));
      wrapped[at][3] = 0;
    }
    IndexedRasterizer rasterizer(countedLeading, countedTrailing);
    IndexedFrame frame;
    std::vector<uint8_t> shownPixels;
    int offset = 0;
    int pannedFrames = 0;
    int shiftedRows = 0;
    int drawnPixels = 0;
    int pannedScans = 0;
    int stillChanges = 0;
    for (int step = 0; step < 60; ++step) {
      const bool pans = step < 50;
      const bool fades = step >= 54;
      const int moved = pans ? 1 + step % 2 : 0;
      offset += moved;
      Layer &shown = display.layers[0];
      shown.pixels = buffers[static_cast<std::size_t>(step % 2)].data();
      shown.sourceX = offset;
      shown.sprites = {
          {title.data(), 16, 4, std::min(offset, 60) + TITLE_X, 4},
          {hands[static_cast<std::size_t>(offset / 4 % 2)].data(), 8, 4, 70,
           12},
          {wrapped[static_cast<std::size_t>(offset / 5 % 5)].data(), 8, 2, 2,
           2}};
      if (fades) {
        for (uint16_t &color : shown.palette) {
          color = static_cast<uint16_t>(color & 0xEEE) >> 1;
        }
      }
      g_scans = 0;
      rasterizer.rasterize(display, frame);
      REQUIRE(colorsOf(frame) == expected(display));
      applyChanges(frame, shownPixels);
      REQUIRE(shownPixels == frame.pixels);
      if (pans && step > 0) {
        ++pannedFrames;
        pannedScans += g_scans;
        for (const RowChange &change : frame.changes) {
          shiftedRows += change.shift == -moved ? 1 : 0;
          for (const Span &span : change.spans) {
            drawnPixels += std::max(0, span.last - span.first);
          }
        }
      }
      if (!pans) {
        stillChanges += static_cast<int>(std::count_if(
            frame.changes.begin(), frame.changes.end(), isChanged));
      }
    }

    THEN("Panned frames compare no picture rows and draw only a few columns") {
      REQUIRE(pannedScans == 0);
      REQUIRE(shiftedRows == pannedFrames * SOURCE_HEIGHT);
      REQUIRE(drawnPixels < pannedFrames * SOURCE_HEIGHT * WIDTH / 3);
    }

    THEN("Frames that only change colours or nothing redraw no row") {
      REQUIRE(stillChanges == 0);
    }
  }

  GIVEN("A wrapped picture shown wider than itself with a tall sprite") {
    const std::vector<uint8_t> picture = pattern(20, 12, 16);
    const std::vector<uint8_t> image = pattern(4, 6, 13);
    const std::vector<uint8_t> other = pattern(4, 6, 11);
    Display display = screen(37, 10);
    Layer wrapped = layer(picture, 20, 12, LEVEL_COLORS);
    wrapped.columns = 37;
    wrapped.rows = 10;
    wrapped.sourceX = 5;
    wrapped.wrap = true;
    wrapped.carriesSprites = true;
    wrapped.sprites = {{image.data(), 4, 6, 8, 2}};
    display.layers.push_back(wrapped);
    IndexedRasterizer rasterizer;
    IndexedFrame frame;
    std::vector<uint8_t> shownPixels;
    rasterizer.rasterize(display, frame);
    applyChanges(frame, shownPixels);

    WHEN("The sprite changes where each row shows it twice") {
      display.layers[0].sprites = {{other.data(), 4, 6, 9, 2}};
      rasterizer.rasterize(display, frame);
      applyChanges(frame, shownPixels);

      THEN("Both of its copies change") {
        REQUIRE(colorsOf(frame) == expected(display));
        REQUIRE(shownPixels == frame.pixels);
      }
    }
  }

  GIVEN("A street that scrolls left under walking sprites") {
    Random random;
    std::vector<uint8_t> street = pattern(64, 24, 16);
    std::vector<std::vector<uint8_t>> images;
    Display display = screen(48, 24);
    Layer streetLayer = layer(street, 64, 24, LEVEL_COLORS);
    streetLayer.sourceX = 8;
    streetLayer.columns = 48;
    streetLayer.carriesSprites = true;
    streetLayer.revision = 1;
    display.layers.push_back(streetLayer);
    for (int at = 0; at < 3; ++at) {
      display.layers[0].sprites.push_back(
          {spriteImage(random, 8, 10, images), 8, 10, 12 + 14 * at, 6 + at});
    }
    IndexedRasterizer rasterizer(countedLeading, countedTrailing);
    IndexedFrame frame;
    std::vector<uint8_t> shownPixels;
    int shiftedRows = 0;
    int stillScans = 0;
    for (int step = 0; step < 12; ++step) {
      if (step % 2 == 1) {
        for (int y = 0; y < 24; ++y) {
          uint8_t *row = street.data() + y * 64;
          std::memmove(row, row + SHIFT_STEP, 64 - SHIFT_STEP);
          std::fill(row + 64 - SHIFT_STEP, row + 64,
                    static_cast<uint8_t>(y % 16));
        }
        ++display.layers[0].revision;
      }
      std::vector<Sprite> &sprites = display.layers[0].sprites;
      sprites[0].left += 1;
      sprites[1].top = 6 + step % 3;
      if (step % 3 == 2) {
        sprites[2].pixels = spriteImage(random, 8, 10, images);
      }
      g_scans = 0;
      rasterizer.rasterize(display, frame);
      REQUIRE(colorsOf(frame) == expected(display));
      applyChanges(frame, shownPixels);
      REQUIRE(shownPixels == frame.pixels);
      if (step > 0 && step % 2 == 0) {
        stillScans += g_scans;
      }
      if (step % 2 == 1) {
        shiftedRows += static_cast<int>(
            std::count_if(frame.changes.begin(), frame.changes.end(),
                          [](const RowChange &change) {
                            return change.shift == -SHIFT_STEP;
                          }));
      }
    }

    THEN("Scrolled frames move the rows under the sprites too") {
      REQUIRE(shiftedRows > 6 * 24 / 2);
    }

    THEN("Frames that only move sprites compare no street rows") {
      REQUIRE(stillScans == 0);
    }
  }
}

SCENARIO("IndexedRasterizer keeps a sprite pinned over a panning picture") {
  GIVEN("A wrapped picture with copper colours and a mostly clear title "
        "pinned in place") {
    constexpr int SOURCE_WIDTH = 120;
    constexpr int SOURCE_HEIGHT = 24;
    constexpr int WIDTH = 40;
    std::vector<uint8_t> picture = pattern(SOURCE_WIDTH, SOURCE_HEIGHT, 16);
    std::vector<uint8_t> title(20 * 9);
    for (int y = 0; y < 9; ++y) {
      for (int x = 0; x < 20; ++x) {
        uint8_t value = 0;
        if (x < 4) {
          value = static_cast<uint8_t>(1 + (x + y) % 4);
        } else if (x >= 8 && x < 12) {
          value = (x + y) % 2 == 0 ? 5 : 0;
        } else if (x >= 12 && x < 16) {
          value = x == 12 + y % 4 ? 6 : 0;
        } else if (x >= 16) {
          value = static_cast<uint8_t>(7 + (x + y) % 3);
        }
        title[static_cast<std::size_t>(y * 20 + x)] = value;
      }
    }
    const std::vector<uint8_t> other = pattern(4, 3, 9);
    std::vector<uint16_t> colors;
    for (int color = 0; color < 16; ++color) {
      colors.push_back(static_cast<uint16_t>(0x111 * color + color % 3));
    }
    Display display = screen(WIDTH, SOURCE_HEIGHT);
    Layer graveyard = layer(picture, SOURCE_WIDTH, SOURCE_HEIGHT, colors);
    graveyard.columns = WIDTH;
    graveyard.wrap = true;
    graveyard.sourceX = 30;
    graveyard.revision = 1;
    graveyard.carriesSprites = true;
    for (int row = 0; row < SOURCE_HEIGHT; row += 2) {
      graveyard.rowColors.push_back(
          {row, static_cast<uint8_t>(row % 5),
           static_cast<uint16_t>(0x00F + 0x100 * (row % 7))});
    }
    display.layers.push_back(graveyard);
    IndexedRasterizer rasterizer;
    IndexedFrame frame;
    std::vector<uint8_t> shownPixels;
    int titleX = 10;
    const auto show = [&] {
      Layer &shown = display.layers[0];
      shown.sprites.resize(std::max<std::size_t>(shown.sprites.size(), 1));
      shown.sprites[0] = {title.data(), 20, 9, shown.sourceX + titleX, 6};
      rasterizer.rasterize(display, frame);
      applyChanges(frame, shownPixels);
      REQUIRE(colorsOf(frame) == expected(display));
      REQUIRE(shownPixels == frame.pixels);
    };
    const auto pan = [&](int moved) {
      display.layers[0].sourceX += moved;
      show();
    };
    show();

    WHEN("The picture pans left and right under it") {
      for (int step = 0; step < 12; ++step) {
        pan(1 + step % 2);
      }
      for (int step = 0; step < 12; ++step) {
        pan(-(1 + step % 3));
      }

      THEN("Every frame shows the title over the moved picture") {
        REQUIRE(colorsOf(frame) == expected(display));
      }
    }

    WHEN("Another sprite crosses the title while the picture pans") {
      display.layers[0].sprites.push_back(
          {other.data(), 4, 3, display.layers[0].sourceX + 6, 8});
      for (int step = 0; step < 10; ++step) {
        display.layers[0].sprites[1].left += 3;
        pan(1);
      }

      THEN("Every frame shows both sprites") {
        REQUIRE(colorsOf(frame) == expected(display));
      }
    }

    WHEN("A sprite appears inside the title while the picture pans") {
      pan(1);
      display.layers[0].sprites.push_back(
          {other.data(), 4, 3, display.layers[0].sourceX + titleX + 6, 8});
      pan(1);
      display.layers[0].sprites[1].left += 1;
      pan(1);

      THEN("It is shown over the title") {
        REQUIRE(colorsOf(frame) == expected(display));
      }
    }

    WHEN("A sprite inside the title goes while the picture pans") {
      display.layers[0].sprites.push_back(
          {other.data(), 4, 3, display.layers[0].sourceX + titleX + 6, 8});
      show();
      display.layers[0].sprites.pop_back();
      pan(2);

      THEN("The title is shown without it") {
        REQUIRE(colorsOf(frame) == expected(display));
      }
    }

    WHEN("A sprite on the title's rows beside it changes while the picture "
         "pans") {
      display.layers[0].sprites.push_back(
          {other.data(), 4, 3, display.layers[0].sourceX + 34, 7});
      for (int step = 0; step < 6; ++step) {
        display.layers[0].sprites[1].top = 7 + step % 2;
        pan(2);
      }

      THEN("Every frame shows both sprites") {
        REQUIRE(colorsOf(frame) == expected(display));
      }
    }

    WHEN("The title is pinned against the edge where the picture comes in") {
      titleX = 19;
      show();
      for (int step = 0; step < 4; ++step) {
        pan(1 + step % 2);
      }
      titleX = -5;
      show();
      for (int step = 0; step < 4; ++step) {
        pan(-1);
      }

      THEN("Every frame shows the title") {
        REQUIRE(colorsOf(frame) == expected(display));
      }
    }

    WHEN("The picture changes under the title while it pans") {
      for (int step = 0; step < 6; ++step) {
        const int column = display.layers[0].sourceX + 1 + titleX + 5;
        for (int y = 5; y < 16; ++y) {
          picture[static_cast<std::size_t>(y * SOURCE_WIDTH + column)] ^= 3;
        }
        ++display.layers[0].revision;
        pan(1);
      }

      THEN("Every frame shows the changed picture under the title") {
        REQUIRE(colorsOf(frame) == expected(display));
      }
    }
  }
}

SCENARIO("IndexedRasterizer pans layers in all combinations") {
  GIVEN("A play screen over a wider picture and a panel below it") {
    std::vector<uint8_t> play = pattern(40, 8, 16);
    const std::vector<uint8_t> panel = pattern(36, 3, 8);
    Display display = screen(32, 11, 0x0F0);
    Layer playLayer = layer(play, 40, 8, LEVEL_COLORS);
    playLayer.columns = 32;
    playLayer.sourceX = 4;
    display.layers.push_back(playLayer);
    Layer panelLayer = layer(panel, 36, 3, PANEL_COLORS);
    panelLayer.top = 8;
    panelLayer.columns = 32;
    panelLayer.sourceX = 2;
    display.layers.push_back(panelLayer);
    IndexedRasterizer rasterizer;
    IndexedFrame frame;
    std::vector<uint8_t> shownPixels;
    const auto show = [&] {
      rasterizer.rasterize(display, frame);
      applyChanges(frame, shownPixels);
    };
    show();

    WHEN("Both layers pan in the same frame") {
      display.layers[0].sourceX += 1;
      display.layers[1].sourceX -= 1;
      show();

      THEN("Both are shown where they moved to") {
        REQUIRE(colorsOf(frame) == expected(display));
        REQUIRE(shownPixels == frame.pixels);
      }
    }

    WHEN("The play screen pans while its picture changes at one end and a "
         "sprite at the other") {
      const std::vector<uint8_t> first = pattern(4, 3, 9);
      const std::vector<uint8_t> second = pattern(4, 3, 7);
      display.layers[0].carriesSprites = true;
      display.layers[0].sprites = {{first.data(), 4, 3, 32, 2}};
      show();
      display.layers[0].sourceX += 1;
      display.layers[0].sprites = {{second.data(), 4, 3, 32, 2}};
      for (int y = 2; y < 5; ++y) {
        play[static_cast<std::size_t>(y * 40 + 6)] ^= 1;
      }
      show();

      THEN("Both changes are shown") {
        REQUIRE(colorsOf(frame) == expected(display));
        REQUIRE(shownPixels == frame.pixels);
      }
    }

    WHEN("The play screen pans past the end of its picture") {
      display.layers[0].sourceX = 8;
      show();
      display.layers[0].sourceX = 9;
      show();

      THEN("The column it no longer covers shows the border") {
        REQUIRE(colorsOf(frame) == expected(display));
        REQUIRE(shownPixels == frame.pixels);
      }
    }
  }
}

SCENARIO("IndexedRasterizer redraws copper rows whose colours change") {
  GIVEN("A wrapped picture with copper colours, one of them a picture colour") {
    std::vector<uint8_t> pixels = pattern(24, 6, 8);
    for (std::size_t at = 0; at < pixels.size(); at += 2) {
      pixels[at] = 0;
    }
    Display display = screen(16, 6, 0x000);
    Layer picture =
        layer(pixels, 24, 6,
              {0x000, 0x0F0, 0x00F, 0x888, 0xF0F, 0x0FF, 0xFF0, 0x444});
    picture.columns = 16;
    picture.wrap = true;
    picture.sourceX = 3;
    picture.rowColors = {{0, 0, 0x0F0}, {1, 0, 0xF00}, {2, 0, 0x0F0},
                         {3, 0, 0x123}, {4, 0, 0xF00}, {5, 0, 0x456}};
    display.layers.push_back(picture);
    IndexedRasterizer rasterizer;
    IndexedFrame frame;
    std::vector<uint8_t> shownPixels;
    const auto show = [&] {
      rasterizer.rasterize(display, frame);
      applyChanges(frame, shownPixels);
    };
    show();
    Layer &shown = display.layers[0];

    WHEN("Only the copper colours change, so rows stop sharing them") {
      shown.rowColors.begin()[2].color = 0x0A0;
      shown.rowColors.begin()[4].color = 0xF55;
      show();

      THEN("Every row shows its new colour") {
        REQUIRE(colorsOf(frame) == expected(display));
        REQUIRE(shownPixels == frame.pixels);
      }
    }

    WHEN("The picture pans while the copper colours change") {
      shown.sourceX += 1;
      shown.rowColors.begin()[2].color = 0x0A0;
      shown.rowColors.begin()[4].color = 0xF55;
      show();

      THEN("The moved rows show the new colours") {
        REQUIRE(colorsOf(frame) == expected(display));
        REQUIRE(shownPixels == frame.pixels);
      }
    }

    WHEN("The picture pans right past its left edge") {
      shown.sourceX = 1;
      show();
      shown.sourceX = -1;
      show();

      THEN("The uncovered columns show each row's copper colour") {
        REQUIRE(colorsOf(frame) == expected(display));
        REQUIRE(shownPixels == frame.pixels);
      }
    }

    WHEN("The copper changes move to other colours and rows") {
      shown.rowColors = {{0, 1, 0xF00},
                         {1, 2, 0x0F0},
                         {2, 1, 0x00F},
                         {3, 2, 0xFF0},
                         {4, 1, 0x0FF}};
      show();

      THEN("The rows that lost their copper colours show the picture's") {
        REQUIRE(colorsOf(frame) == expected(display));
        REQUIRE(shownPixels == frame.pixels);
      }
    }

    WHEN("The picture pans under a sprite on rows with their own copper "
         "colours") {
      shown.rowColors = {{0, 1, 0xF00},
                         {1, 2, 0x0F0},
                         {2, 1, 0x00F},
                         {3, 2, 0xFF0},
                         {4, 1, 0x0FF}};
      std::vector<uint8_t> image(4 * 5);
      for (std::size_t at = 0; at < image.size(); ++at) {
        image[at] = static_cast<uint8_t>(1 + at % 2);
      }
      shown.carriesSprites = true;
      shown.sprites = {{image.data(), 4, 5, 17, 0}};
      show();
      shown.sourceX += 1;
      shown.sprites[0].left += 1;
      show();

      THEN("The sprite shows each row's copper colours") {
        REQUIRE(colorsOf(frame) == expected(display));
        REQUIRE(shownPixels == frame.pixels);
      }
    }

    WHEN("The picture colour that a copper row shares fades") {
      shown.palette[1] = 0x080;
      show();

      THEN("The copper rows keep their colour") {
        REQUIRE(colorsOf(frame) == expected(display));
        REQUIRE(shownPixels == frame.pixels);
      }
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
