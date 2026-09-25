#include "../../../src/systems/Display.h"
#include <catch2/catch_all.hpp>

#include <cstdint>
#include <vector>

using namespace openfranko::src::systems;

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
