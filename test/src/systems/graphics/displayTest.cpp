#include "../../../../src/systems/graphics/Display.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <vector>

using namespace openfranko::src::systems::graphics;

namespace {

constexpr uint32_t BORDER = 0xFF555555u;
const std::vector<uint16_t> COLORS = {0x000, 0x100, 0x200, 0x300,
                                      0x400, 0x500, 0x600, 0x700};
const std::vector<uint8_t> SOURCE = {1, 2, 3, 4, 5, 6};

uint32_t color(int index) { return toArgb(COLORS[index]); }

Display display(int width, int height) {
  Display output;
  output.width = width;
  output.height = height;
  output.displayHeight = height;
  output.border = 0x555;
  return output;
}

Layer source(int columns, int rows) {
  Layer layer;
  layer.pixels = SOURCE.data();
  layer.stride = 3;
  layer.sourceColumns = 3;
  layer.sourceRows = 2;
  layer.columns = columns;
  layer.rows = rows;
  layer.palette = COLORS;
  return layer;
}

std::vector<uint32_t> shown(const Display &output) {
  std::vector<uint32_t> argb;
  rasterize(output, argb);
  return argb;
}

} // namespace

SCENARIO("rasterize paints the border and then each layer in turn") {
  GIVEN("A 3 x 2 display") {
    Display output = display(3, 2);

    THEN("With no layers every pixel is the border") {
      REQUIRE(shown(output) == std::vector<uint32_t>(6, BORDER));
    }

    THEN("A solid layer paints its rows") {
      output.layers.push_back(solidLayer(0xF00, 1, 1, 2));
      REQUIRE(shown(output) == std::vector<uint32_t>{BORDER, BORDER, BORDER,
                                                     0xFFFF0000u, 0xFFFF0000u,
                                                     BORDER});
    }

    THEN("A layer maps each source pixel through its palette") {
      output.layers.push_back(source(3, 2));
      REQUIRE(shown(output) == std::vector<uint32_t>{color(1), color(2),
                                                     color(3), color(4),
                                                     color(5), color(6)});
    }

    THEN("A later layer covers an earlier one") {
      output.layers.push_back(source(3, 2));
      output.layers.push_back(solidLayer(0xF00, 0, 1, 3));
      REQUIRE(shown(output)[0] == 0xFFFF0000u);
      REQUIRE(shown(output)[3] == color(4));
    }

    THEN("Scrolled past the source width, the layer shows what is below") {
      Layer layer = source(3, 2);
      layer.sourceX = 1;
      output.layers.push_back(layer);
      REQUIRE(shown(output) == std::vector<uint32_t>{color(2), color(3), BORDER,
                                                     color(5), color(6),
                                                     BORDER});
    }

    THEN("A wrapping layer goes on with the next row, then colour 0") {
      Layer layer = source(3, 2);
      layer.sourceX = 1;
      layer.wrap = true;
      output.layers.push_back(layer);
      REQUIRE(shown(output) == std::vector<uint32_t>{color(2), color(3),
                                                     color(4), color(5),
                                                     color(6), color(0)});
    }

    THEN("The mask limits the colour numbers") {
      Layer layer = source(3, 2);
      layer.mask = 3;
      output.layers.push_back(layer);
      REQUIRE(shown(output)[3] == color(0));
      REQUIRE(shown(output)[5] == color(2));
    }

    THEN("A row colour changes one entry on one row") {
      Layer layer = source(3, 2);
      layer.rowColors.push_back({1, 5, 0xFFF});
      output.layers.push_back(layer);
      REQUIRE(shown(output)[4] == 0xFFFFFFFFu);
      REQUIRE(shown(output)[1] == color(2));
    }
  }

  GIVEN("A 1 x 4 display") {
    Display output = display(1, 4);

    THEN("Repeat shows each source row on several rows") {
      Layer layer = source(1, 4);
      layer.repeat = 2;
      output.layers.push_back(layer);
      REQUIRE(shown(output) ==
              std::vector<uint32_t>{color(1), color(1), color(4), color(4)});
    }

    THEN("A source step skips rows, and rows past the source show below") {
      Layer layer = source(1, 4);
      layer.sourceStep = 2;
      layer.sourceY = -2;
      output.layers.push_back(layer);
      REQUIRE(shown(output) ==
              std::vector<uint32_t>{BORDER, color(1), BORDER, BORDER});
    }
  }
}

SCENARIO("cropRows keeps a band of rows") {
  GIVEN("A display with a layer at row 5") {
    Display output = display(3, 10);
    output.layers.push_back(solidLayer(0xF00, 5, 2, 3));

    WHEN("Rows 4 to 6 are kept") {
      cropRows(output, 4, 3);

      THEN("The layer moves up with them") {
        REQUIRE(output.height == 3);
        REQUIRE(output.displayHeight == 3);
        REQUIRE(output.layers[0].top == 1);
      }
    }
  }
}

SCENARIO("Row colours are shared until a copy changes them") {
  GIVEN("Row colours and a copy of them") {
    RowColors rows = {{1, 0, 0xF00}, {2, 3, 0x0F0}};
    RowColors copy = rows;

    THEN("The copy shares the same rows") {
      REQUIRE(copy.shares(rows));
      REQUIRE(copy.size() == 2);
    }

    WHEN("The copy is changed") {
      for (RowColor &row : copy) {
        row.color = 0x00F;
      }

      THEN("Only the copy changes") {
        REQUIRE_FALSE(copy.shares(rows));
        REQUIRE(rows.begin()->color == 0xF00);
        REQUIRE(copy.begin()->color == 0x00F);
      }
    }

    WHEN("A row is added to the copy and the original is cleared") {
      copy.push_back({5, 1, 0x123});
      rows.clear();

      THEN("Each keeps its own rows") {
        REQUIRE(copy.size() == 3);
        REQUIRE(rows.empty());
        REQUIRE(rows.begin() == rows.end());
      }
    }
  }
}

SCENARIO("Sprites are drawn over their layer inside its source") {
  GIVEN("A scrolled screen and three overlapping sprites") {
    const std::vector<uint8_t> screenPixels(6, 1);
    const std::vector<uint8_t> first{2, 2, 3, 0};
    const std::vector<uint8_t> second{6, 0};
    const std::vector<uint8_t> third{0, 7, 2};
    Display display;
    display.width = 6;
    display.height = 1;
    display.displayHeight = 1;
    display.border = 0x444;
    Layer screen;
    screen.pixels = screenPixels.data();
    screen.stride = 6;
    screen.sourceColumns = 6;
    screen.sourceRows = 1;
    screen.sourceX = 1;
    screen.columns = 6;
    screen.rows = 1;
    screen.mask = 0x03;
    screen.palette = {0x000, 0x00F, 0x0F0, 0xF00};
    screen.carriesSprites = true;
    screen.sprites = {{first.data(), 4, 1, -1, 0},
                      {second.data(), 2, 1, 1, 0},
                      {third.data(), 3, 1, 4, 0}};
    display.layers = {screen};

    THEN("Later sprites cover earlier ones, colour 0 is see-through and "
         "nothing is drawn outside the screen's pixels") {
      std::vector<uint32_t> argb;
      rasterize(display, argb);
      REQUIRE(argb[0] == toArgb(0x0F0));
      REQUIRE(argb[1] == toArgb(0x00F));
      REQUIRE(argb[2] == toArgb(0x00F));
      REQUIRE(argb[3] == toArgb(0x00F));
      REQUIRE(argb[4] == toArgb(0xF00));
      REQUIRE(argb[5] == toArgb(0x444));
    }
  }
}

namespace {

bool sameLayers(const Display &left, const Display &right) {
  if (left.width != right.width || left.height != right.height ||
      left.displayHeight != right.displayHeight ||
      left.border != right.border || left.revision != right.revision ||
      left.layers.size() != right.layers.size()) {
    return false;
  }
  for (std::size_t index = 0; index < left.layers.size(); ++index) {
    const Layer &a = left.layers[index];
    const Layer &b = right.layers[index];
    if (a.pixels != b.pixels || a.stride != b.stride ||
        a.sourceColumns != b.sourceColumns || a.sourceRows != b.sourceRows ||
        a.sourceX != b.sourceX || a.sourceY != b.sourceY ||
        a.sourceStep != b.sourceStep || a.repeat != b.repeat ||
        a.wrap != b.wrap || a.left != b.left || a.top != b.top ||
        a.columns != b.columns || a.rows != b.rows || a.mask != b.mask ||
        a.palette != b.palette || a.rowColors.size() != b.rowColors.size() ||
        a.revision != b.revision || a.carriesSprites != b.carriesSprites ||
        a.sprites != b.sprites) {
      return false;
    }
  }
  return true;
}

} // namespace

SCENARIO("assign copies a display as a plain copy would") {
  GIVEN("Two displays with different layers") {
    const std::vector<uint8_t> pixels(64, 3);
    Display first;
    first.width = 8;
    first.height = 8;
    first.displayHeight = 8;
    first.border = 0x123;
    first.revision = 7;
    Layer screen;
    screen.pixels = pixels.data();
    screen.stride = 8;
    screen.sourceColumns = 8;
    screen.sourceRows = 8;
    screen.columns = 8;
    screen.rows = 8;
    screen.palette = {0x000, 0x111, 0x222, 0x333};
    screen.rowColors = RowColors{{1, 2, 0xF00}};
    screen.carriesSprites = true;
    screen.sprites = {{pixels.data(), 8, 2, 1, 3}};
    Layer panel = screen;
    panel.carriesSprites = false;
    panel.sprites.clear();
    panel.sourceX = -2;
    first.layers = {screen, panel, solidLayer(0x0F0, 2, 3, 8)};
    Display second = first;
    second.revision = 9;
    second.layers[0].palette = {0xFFF};
    second.layers[0].sprites.push_back({pixels.data(), 8, 1, -4, 0});
    second.layers[1].sourceY = 5;
    second.layers[1].carriesSprites = true;
    second.layers[2].palette.clear();

    THEN("Assigning either way, or to a different layer count, matches") {
      Display target = first;
      assign(target, second);
      REQUIRE(sameLayers(target, second));
      assign(target, first);
      REQUIRE(sameLayers(target, first));
      Display fewer;
      fewer.layers.push_back(screen);
      assign(fewer, second);
      REQUIRE(sameLayers(fewer, second));
    }

    THEN("Row colours that differ are taken over and then shared") {
      second.layers[0].rowColors.push_back({4, 1, 0x0F0});
      Display target = first;
      REQUIRE_FALSE(
          target.layers[0].rowColors.shares(second.layers[0].rowColors));
      assign(target, second);
      REQUIRE(sameLayers(target, second));
      REQUIRE(target.layers[0].rowColors.shares(second.layers[0].rowColors));
      REQUIRE(target.layers[1].rowColors.shares(first.layers[1].rowColors));
    }
  }
}
