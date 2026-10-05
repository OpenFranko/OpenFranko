#include "../../../../../src/engine/street/core/SystemText.h"

#include <catch2/catch_all.hpp>

#include <vector>

using namespace openfranko::src::engine::street::core;

namespace {

constexpr uint8_t BACKGROUND = 7;
constexpr uint8_t INK = 9;
constexpr uint8_t PAPER = 2;
constexpr int X = 8;
constexpr int BASELINE = 10;
constexpr int TOP = BASELINE - SYSTEM_FONT_BASELINE;

int countInCell(const IndexedSurface &screen, int cell, uint8_t color) {
  int count = 0;
  for (int y = TOP; y < TOP + SYSTEM_FONT_HEIGHT; ++y) {
    for (int x = X + cell * SYSTEM_FONT_WIDTH;
         x < X + (cell + 1) * SYSTEM_FONT_WIDTH; ++x) {
      count += screen.pixel(x, y) == color ? 1 : 0;
    }
  }
  return count;
}

} // namespace

SCENARIO("Text writes 8 x 8 cells of ink on paper from its baseline") {
  GIVEN("A screen filled with a background colour") {
    IndexedSurface screen(64, 24);
    screen.fill(BACKGROUND);

    WHEN("KOD is written") {
      drawSystemText(screen, X, BASELINE, "KOD", INK, PAPER);

      THEN("Each cell holds only ink and paper, six rows above the baseline "
           "and one below") {
        for (int cell = 0; cell < 3; ++cell) {
          REQUIRE(countInCell(screen, cell, INK) > 0);
          REQUIRE(countInCell(screen, cell, INK) +
                      countInCell(screen, cell, PAPER) ==
                  SYSTEM_FONT_WIDTH * SYSTEM_FONT_HEIGHT);
        }
        REQUIRE(screen.pixel(X, TOP - 1) == BACKGROUND);
        REQUIRE(screen.pixel(X, TOP + SYSTEM_FONT_HEIGHT) == BACKGROUND);
        REQUIRE(screen.pixel(X - 1, TOP) == BACKGROUND);
        REQUIRE(screen.pixel(X + 3 * SYSTEM_FONT_WIDTH, TOP) == BACKGROUND);
      }
    }

    WHEN("A character the font lacks is written between two it has") {
      drawSystemText(screen, X, BASELINE, "K?D", INK, PAPER);

      THEN("Its cell is blank paper") {
        REQUIRE(countInCell(screen, 0, INK) > 0);
        REQUIRE(countInCell(screen, 1, PAPER) ==
                SYSTEM_FONT_WIDTH * SYSTEM_FONT_HEIGHT);
        REQUIRE(countInCell(screen, 2, INK) > 0);
      }
    }

    WHEN("Empty text is written") {
      const std::vector<uint8_t> before = screen.pixels();
      drawSystemText(screen, X, BASELINE, "", INK, PAPER);

      THEN("The screen is untouched") { REQUIRE(screen.pixels() == before); }
    }
  }
}
