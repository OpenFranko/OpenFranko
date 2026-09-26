#include "../../../../src/engine/effects/BlyskSequence.h"
#include <catch2/catch_all.hpp>

using namespace openfranko::src::engine::effects;

namespace {

void run(BlyskSequence &sequence, int frames, bool fireLatched = false) {
  for (int frame = 0; frame < frames; ++frame) {
    sequence.advance(fireLatched);
  }
}

} // namespace

SCENARIO("BlyskSequence shows FONT pages the way BLYSK lights them") {
  GIVEN("The two pages shown before the knee") {
    BlyskSequence sequence(0, 2);

    WHEN("The first frame runs") {
      run(sequence, 1);

      THEN("The first page is pasted on the still black strip") {
        REQUIRE(sequence.page() == 0);
        REQUIRE(sequence.palette() == AmigaPalette{0x000, 0x000, 0x000, 0x000});
      }
    }

    WHEN("Fade 2 has had 29 frames") {
      run(sequence, 30);

      THEN("Ink and shade have reached white and grey, colour 3 is kept") {
        REQUIRE(sequence.palette() == AmigaPalette{0x000, 0xFFF, 0xAAA, 0x000});
      }
    }

    WHEN("Wait 60 has passed") {
      run(sequence, 61);

      THEN("Fade 2,0,0,0 has started but not stepped yet") {
        REQUIRE(sequence.palette()[1] == 0xFFF);
      }

      AND_WHEN("The next VBL comes") {
        run(sequence, 1);

        THEN("The page takes its first step back to black") {
          REQUIRE(sequence.palette()[1] == 0xEEE);
          REQUIRE(sequence.palette()[2] == 0x999);
        }
      }
    }

    WHEN("Wait 30 has passed after the fade out") {
      run(sequence, 90);

      THEN("The first page is still up") {
        REQUIRE(sequence.page() == 0);
        REQUIRE(sequence.palette() == AmigaPalette{0x000, 0x000, 0x000, 0x000});
      }

      AND_WHEN("Cls 0 runs") {
        run(sequence, 1);

        THEN("The second page is pasted in the same frame") {
          REQUIRE(sequence.page() == 1);
          REQUIRE_FALSE(sequence.isFinished());
        }
      }
    }

    WHEN("Both pages have been shown") {
      run(sequence, 2 * BlyskSequence::PAGE_FRAMES + 1);

      THEN("The strip is clear and the sequence is over") {
        REQUIRE(sequence.isFinished());
        REQUIRE_FALSE(sequence.isSkipped());
        REQUIRE_FALSE(sequence.page().has_value());
      }
    }

    WHEN("Fire is latched while the first page is up") {
      run(sequence, BlyskSequence::PAGE_FRAMES);
      run(sequence, 1, true);

      THEN("Exit If Amreg(25)=1 ends the sequence after its Cls") {
        REQUIRE(sequence.isFinished());
        REQUIRE(sequence.isSkipped());
        REQUIRE_FALSE(sequence.page().has_value());
      }
    }
  }

  GIVEN("No pages") {
    BlyskSequence sequence(2, 2);

    THEN("There is nothing to show") {
      REQUIRE(sequence.isFinished());
      REQUIRE_FALSE(sequence.isSkipped());
    }
  }
}
