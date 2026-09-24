#include "../../../../src/engine/street/StatusPanel.h"
#include <catch2/catch_all.hpp>

using namespace openfranko::src::engine::street;

namespace {

constexpr uint8_t PANEL_COLOR = 1;
constexpr uint8_t STRIP_BOTTOM = 99;

uint8_t cell(int x) { return static_cast<uint8_t>(x / 8 + 10); }

Picture artwork() {
  Picture picture{304, 40, 0, 0, {}};
  for (int y = 0; y < 40; ++y) {
    for (int x = 0; x < 304; ++x) {
      picture.pixels.push_back(y >= 32 ? cell(x) : PANEL_COLOR);
    }
  }
  return picture;
}

Picture loadingStrip() {
  Picture picture{304, 48, 0, 0, {}};
  for (int y = 0; y < 48; ++y) {
    for (int x = 0; x < 304; ++x) {
      picture.pixels.push_back(y >= 40 ? STRIP_BOTTOM
                                       : static_cast<uint8_t>(50 + x % 7));
    }
  }
  return picture;
}

StatusPanel::Stats stats(int energy, int stage, int kills, int lives) {
  StatusPanel::Stats result;
  result.energy = energy;
  result.stage = stage;
  result.kills = kills;
  result.lives = lives;
  return result;
}

} // namespace

SCENARIO("CZEKAJ shows the loading strip with its word") {
  GIVEN("A panel") {
    StatusPanel panel(loadingStrip(), artwork());
    panel.showLoading();

    THEN("The strip fills all 48 rows and the word is copied to (101, 10)") {
      REQUIRE(panel.surface().pixel(0, 47) == STRIP_BOTTOM);
      REQUIRE(panel.surface().pixel(101, 10) == 50 + 203 % 7);
      REQUIRE(panel.surface().pixel(200, 17) == 50 + 302 % 7);
    }
  }
}

SCENARIO("SCORE redraws the panel from its own glyph atlas") {
  GIVEN("A panel that has shown the loading strip") {
    StatusPanel panel(loadingStrip(), artwork());
    panel.showLoading();
    const IndexedSurface &surface = panel.surface();

    WHEN("Stage 1 is drawn with 60 energy, 7 kills and 3 lives") {
      panel.score(stats(60, 1, 7, 3));

      THEN("The artwork covers rows 0 to 39 and the strip stays below") {
        REQUIRE(surface.pixel(0, 0) == PANEL_COLOR);
        REQUIRE(surface.pixel(0, 39) == cell(0));
        REQUIRE(surface.pixel(0, 40) == STRIP_BOTTOM);
      }

      THEN("The bar is erased from 111 + energy to 175 on rows 13 and 14") {
        REQUIRE(surface.pixel(170, 13) == PANEL_COLOR);
        REQUIRE(surface.pixel(171, 13) == 6);
        REQUIRE(surface.pixel(175, 14) == 6);
        REQUIRE(surface.pixel(176, 14) == PANEL_COLOR);
      }

      THEN("The stage digit is a 7 x 7 copy to (48, 9)") {
        REQUIRE(surface.pixel(48, 9) == cell(8));
        REQUIRE(surface.pixel(54, 15) == cell(8));
        REQUIRE(surface.pixel(55, 9) == PANEL_COLOR);
      }

      THEN("One kill digit is centred at x 78") {
        REQUIRE(surface.pixel(77, 9) == PANEL_COLOR);
        REQUIRE(surface.pixel(78, 9) == cell(56));
        REQUIRE(surface.pixel(85, 9) == cell(56));
      }

      THEN("Three life icons span x 6 to 39, their last row from the strip") {
        REQUIRE(surface.pixel(6, 8) == cell(80));
        REQUIRE(surface.pixel(39, 8) == cell(113));
        REQUIRE(surface.pixel(40, 8) == PANEL_COLOR);
        REQUIRE(surface.pixel(6, 16) == STRIP_BOTTOM);
      }
    }

    WHEN("There is one life in reserve") {
      panel.score(stats(64, 1, 0, 1));

      THEN("One icon is copied") {
        REQUIRE(surface.pixel(15, 8) == cell(89));
        REQUIRE(surface.pixel(16, 8) == PANEL_COLOR);
      }
    }

    WHEN("There are none") {
      panel.score(stats(64, 1, 0, 0));

      THEN("No icon is copied") { REQUIRE(surface.pixel(6, 8) == PANEL_COLOR); }
    }

    WHEN("There are five") {
      panel.score(stats(64, 1, 0, 5));

      THEN("Three icons and the digit 5 at (28, 19) are shown") {
        REQUIRE(surface.pixel(39, 8) == cell(113));
        REQUIRE(surface.pixel(28, 19) == cell(40));
      }
    }
  }
}

SCENARIO("WZAB centres the kill counter on x 82") {
  GIVEN("A drawn panel") {
    StatusPanel panel(loadingStrip(), artwork());
    panel.score(stats(64, 1, 0, 3));
    const IndexedSurface &surface = panel.surface();

    THEN("Two digits start at 74") {
      panel.drawKills(12);
      REQUIRE(surface.pixel(74, 9) == cell(8));
      REQUIRE(surface.pixel(82, 9) == cell(16));
    }

    THEN("Three digits start at 70") {
      panel.drawKills(123);
      REQUIRE(surface.pixel(70, 9) == cell(8));
      REQUIRE(surface.pixel(78, 9) == cell(16));
      REQUIRE(surface.pixel(86, 9) == cell(24));
    }

    THEN("Losing energy only erases the bar's end") {
      panel.loseEnergy(40);
      REQUIRE(surface.pixel(150, 13) == PANEL_COLOR);
      REQUIRE(surface.pixel(151, 13) == 6);
    }
  }
}
