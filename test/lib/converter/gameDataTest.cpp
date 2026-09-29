#include "../../../lib/converter/gameData/gameData.h"
#include "../../../lib/converter/gameData/palettes.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <vector>

using namespace openfranko::lib::converter::gameData;

SCENARIO("version10Id names the 1.0 file whose role a 1.2 file has") {
  GIVEN("1.2 files with and without a 1.0 counterpart, and a 1.0 file") {
    THEN("Each maps to its counterpart, new files to nothing") {
      REQUIRE(version10Id("s56") == "0038");
      REQUIRE(version10Id("t40") == "0154");
      REQUIRE(version10Id("p52") == "03B8");
      REQUIRE(version10Id("s50").empty());
      REQUIRE(version10Id("0384") == "0384");
    }
  }
}

SCENARIO("selectPalette returns the correct palette for known file IDs") {
  GIVEN("Known file IDs with specific palettes") {
    WHEN("selectPalette is called with '0038'") {
      auto p = palettes::selectPalette("0038");
      THEN("It returns SUNSET") {
        REQUIRE(p.size() == palettes::SUNSET.size());
        REQUIRE(p[0] == palettes::SUNSET[0]);
        REQUIRE(p[15] == palettes::SUNSET[15]);
      }
    }

    WHEN("selectPalette is called with '0037'") {
      auto p = palettes::selectPalette("0037");
      THEN("It returns STORY") {
        REQUIRE(p.size() == palettes::STORY.size());
        REQUIRE(p[0] == palettes::STORY[0]);
      }
    }

    WHEN("selectPalette is called with '0034'") {
      auto p = palettes::selectPalette("0034");
      THEN("It returns MENU") {
        REQUIRE(p.size() == palettes::MENU.size());
        REQUIRE(p[0] == palettes::MENU[0]);
      }
    }

    WHEN("selectPalette is called with '0035'") {
      auto p = palettes::selectPalette("0035");
      THEN("It returns MENU_35") {
        REQUIRE(p.size() == palettes::MENU_35.size());
        REQUIRE(p[0] == palettes::MENU_35[0]);
      }
    }

    WHEN("selectPalette is called with '0036'") {
      auto p = palettes::selectPalette("0036");
      THEN("It returns CEMETERY") {
        REQUIRE(p.size() == palettes::CEMETERY.size());
        REQUIRE(p[0] == palettes::CEMETERY[0]);
      }
    }

    WHEN("selectPalette is called with the 1.2 counterparts of those files") {
      THEN("They get the same palettes") {
        REQUIRE(palettes::selectPalette("s56")[15] == palettes::SUNSET[15]);
        REQUIRE(palettes::selectPalette("s55").size() ==
                palettes::STORY.size());
        REQUIRE(palettes::selectPalette("s52")[7] == palettes::MENU[7]);
        REQUIRE(palettes::selectPalette("s53").size() ==
                palettes::MENU_35.size());
        REQUIRE(palettes::selectPalette("s54")[2] == palettes::CEMETERY[2]);
      }
    }

    WHEN("selectPalette is called with 's50'") {
      auto p = palettes::selectPalette("s50");
      THEN("It returns WORLD_SOFTWARE") {
        REQUIRE(p.size() == palettes::WORLD_SOFTWARE.size());
        REQUIRE(p[1] == palettes::WORLD_SOFTWARE[1]);
      }
    }

    WHEN("selectPalette is called with an unknown ID") {
      auto p = palettes::selectPalette("9999");
      THEN("It returns LEVEL as default") {
        REQUIRE(p.size() == palettes::LEVEL.size());
        REQUIRE(p[0] == palettes::LEVEL[0]);
      }
    }
  }
}

SCENARIO("byName picks a palette by its command line name") {
  GIVEN("The palette names the tools accept") {
    THEN("Each returns its palette") {
      using Pal = std::vector<uint16_t>;
      REQUIRE(palettes::byName("sunset") ==
              Pal(palettes::SUNSET.begin(), palettes::SUNSET.end()));
      REQUIRE(palettes::byName("story") ==
              Pal(palettes::STORY.begin(), palettes::STORY.end()));
      REQUIRE(palettes::byName("menu") ==
              Pal(palettes::MENU.begin(), palettes::MENU.end()));
      REQUIRE(palettes::byName("menu35") ==
              Pal(palettes::MENU_35.begin(), palettes::MENU_35.end()));
      REQUIRE(palettes::byName("cemetery") ==
              Pal(palettes::CEMETERY.begin(), palettes::CEMETERY.end()));
    }
  }

  GIVEN("\"level\" or an unknown name") {
    THEN("It returns the level palette") {
      using Pal = std::vector<uint16_t>;
      REQUIRE(palettes::byName("level") ==
              Pal(palettes::LEVEL.begin(), palettes::LEVEL.end()));
      REQUIRE(palettes::byName("nope") ==
              Pal(palettes::LEVEL.begin(), palettes::LEVEL.end()));
    }
  }
}
