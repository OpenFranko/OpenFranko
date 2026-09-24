#include "../../../../src/engine/effects/AmigaDisplay.h"
#include <catch2/catch_all.hpp>

using namespace openfranko::src::engine::effects;

namespace {

constexpr int SCREEN_HEIGHT = 256;
constexpr int HISCORE_LINE = 50;
constexpr int ATTRACT_LINE = 40;

} // namespace

SCENARIO("visibleRows clips a screen to what the raster shows") {
  THEN("PAL shows all 256 rows of a screen on line 50") {
    const VisibleRows rows = visibleRows(HISCORE_LINE, SCREEN_HEIGHT, false);
    REQUIRE(rows.first == 0);
    REQUIRE(rows.count == 256);
  }

  THEN("NTSC ends at line 261, so the same screen loses its last 44 rows") {
    const VisibleRows rows = visibleRows(HISCORE_LINE, SCREEN_HEIGHT, true);
    REQUIRE(rows.first == 0);
    REQUIRE(rows.count == 212);
  }

  THEN("Nothing shows above line 26") {
    const VisibleRows rows = visibleRows(13, SCREEN_HEIGHT, true);
    REQUIRE(rows.first == 13);
    REQUIRE(rows.count == 236);
  }
}

SCENARIO("NTSC pictures go 27 lines higher, as the attract's 40+27*SYS") {
  THEN("PAL keeps the line and NTSC raises it") {
    REQUIRE(pictureLine(ATTRACT_LINE, false) == 40);
    REQUIRE(pictureLine(ATTRACT_LINE, true) == 13);
  }

  THEN("Raised, the hiscore screen shows rows 3 to 238") {
    const VisibleRows rows =
        visibleRows(pictureLine(HISCORE_LINE, true), SCREEN_HEIGHT, true);
    REQUIRE(rows.first == 3);
    REQUIRE(rows.count == 236);
  }
}
