#include "../../../../src/engine/effects/Rainbow.h"
#include <catch2/catch_all.hpp>
#include <stdexcept>
#include <vector>

using namespace openfranko::src::engine::effects;

SCENARIO("Set Rainbow builds its table as TRSet does") {
  GIVEN("The game-over sky: red and blue ramps with green left empty") {
    const AmigaPalette table =
        rainbowTable(1000, "(8,-1,15)(16,1,15)", "", "(8,1,15)(16,-1,15)");

    THEN("It has the thousand entries asked for") {
      REQUIRE(table.size() == 1000u);
    }

    THEN("The first move lands on the eighth entry, red wrapping 0 to 15") {
      for (int entry = 0; entry < 8; ++entry) {
        REQUIRE(table[static_cast<std::size_t>(entry)] == 0x000);
      }
      REQUIRE(table[8] == 0xF01);
      REQUIRE(table[16] == 0xE02);
    }

    THEN("Entries 96 to 335 run from deep blue to red") {
      const std::vector<int> offsets = {0, 24, 48, 96, 144, 192, 239};
      const std::vector<AmigaColor> colours = {0x40C, 0x10F, 0x20E, 0x50B,
                                               0x808, 0xB05, 0xE02};
      for (std::size_t i = 0; i < offsets.size(); ++i) {
        REQUIRE(table[static_cast<std::size_t>(96 + offsets[i])] == colours[i]);
      }
    }
  }

  GIVEN("A single move and a starting colour") {
    const AmigaPalette table = rainbowTable(20, "(1,1,2)", "", "", 0x5A3);

    THEN("The list starts over when it runs out and nibbles wrap") {
      for (int entry = 0; entry < 20; ++entry) {
        const int red = (5 + entry) & 0xF;
        REQUIRE(table[static_cast<std::size_t>(entry)] == (red << 8 | 0xA3));
      }
    }
  }

  THEN("A malformed program is refused") {
    REQUIRE_THROWS_AS(rainbowTable(10, "(0,1,1)", "", ""),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(rainbowTable(10, "(1,1)", "", ""), std::invalid_argument);
    REQUIRE_THROWS_AS(rainbowTable(10, "", "(2,1,-1)", ""),
                      std::invalid_argument);
  }
}
