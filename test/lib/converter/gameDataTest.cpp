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
