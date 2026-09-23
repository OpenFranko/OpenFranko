#include "../../../../src/engine/effects/AttractSequence.h"
#include <catch2/catch_all.hpp>

using namespace openfranko::src::engine::effects;

namespace {

const AmigaPalette HISCORE_PALETTE = {
    0x444, 0x400, 0x050, 0x070, 0x700, 0x900, 0x090, 0x540, 0x650, 0x000, 0x555,
    0x04C, 0x666, 0x864, 0x777, 0x09C, 0x975, 0xB95, 0xAAA, 0x6CF, 0xBBB, 0xEC8,
    0xCCC, 0x8FF, 0xDDD, 0xEEE, 0xFFF, 0xFFF, 0x000, 0x760, 0xB95, 0xFC0};

void run(AttractSequence &attract, int frames, bool touched = false) {
  for (int frame = 0; frame < frames; ++frame) {
    attract.advance(touched);
  }
}

} // namespace

SCENARIO("AttractSequence shows the title as the menu's attract loop does") {
  GIVEN("The title") {
    AttractSequence attract(AttractSequence::Kind::Title, HISCORE_PALETTE);

    THEN("It shows at once") {
      attract.advance(false);
      REQUIRE(attract.isShowing());
    }

    WHEN("The joystick is touched during the first Wait 10") {
      run(attract, 10, true);

      THEN("It is ignored") { REQUIRE_FALSE(attract.isFinished()); }

      AND_WHEN("It is touched after the wait") {
        attract.advance(true);

        THEN("The title goes") { REQUIRE(attract.isFinished()); }
      }
    }

    WHEN("Nobody touches the joystick") {
      run(attract, 211);
      const bool finishedBefore = attract.isFinished();
      attract.advance(false);

      THEN("It stays 10 frames plus 201 of Timer>200") {
        REQUIRE_FALSE(finishedBefore);
        REQUIRE(attract.isFinished());
        REQUIRE_FALSE(attract.isShowing());
      }
    }
  }
}

SCENARIO("AttractSequence shows the hi-score table as HISHOW does") {
  GIVEN("The hi-score table") {
    AttractSequence attract(AttractSequence::Kind::Hiscores, HISCORE_PALETTE);

    WHEN("The first frame passes") {
      attract.advance(false);

      THEN("The menu still shows while the first dimming step lands") {
        REQUIRE_FALSE(attract.isShowing());
        REQUIRE(attract.palette()[26] == 0xEEE);
        REQUIRE(attract.rowsShown() == 0);
      }

      AND_WHEN("The next frame passes") {
        attract.advance(false);

        THEN("The table's picture shows") { REQUIRE(attract.isShowing()); }
      }
    }

    WHEN("The four Fade 100 steps are done") {
      run(attract, 16);

      THEN("The picture is four steps darker and no row is drawn") {
        REQUIRE(attract.palette()[26] == 0xBBB);
        REQUIRE(attract.palette()[0] == 0x000);
        REQUIRE(attract.palette()[29] == 0x320);
        REQUIRE(attract.rowsShown() == 0);
      }

      AND_WHEN("The next frame passes") {
        attract.advance(false);

        THEN("Colours 29-31 are relit and the bottom row is drawn") {
          REQUIRE(attract.palette()[29] == 0x769);
          REQUIRE(attract.palette()[30] == 0xB95);
          REQUIRE(attract.palette()[31] == 0xFC0);
          REQUIRE(attract.rowsShown() == 1);
        }
      }
    }

    WHEN("The rows keep coming") {
      run(attract, 17 + 10 * 9);

      THEN("All ten are drawn, one every 10 frames") {
        REQUIRE(attract.rowsShown() == 10);
      }
    }

    WHEN("The joystick is held while the rows are drawn") {
      run(attract, 126, true);

      THEN("HISHOW cannot be interrupted") {
        REQUIRE_FALSE(attract.isFinished());
      }

      AND_WHEN("It is still held when the Timer loop starts") {
        attract.advance(true);

        THEN("The table goes") { REQUIRE(attract.isFinished()); }
      }
    }

    WHEN("Nobody touches the joystick") {
      run(attract, 527);
      const bool finishedBefore = attract.isFinished();
      attract.advance(false);

      THEN("It stays until Timer>400 after the last Wait 10") {
        REQUIRE_FALSE(finishedBefore);
        REQUIRE(attract.isFinished());
      }
    }
  }
}
