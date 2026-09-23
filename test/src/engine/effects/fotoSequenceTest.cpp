#include "../../../../src/engine/effects/FotoSequence.h"
#include <catch2/catch_all.hpp>

using namespace openfranko::src::engine::effects;

namespace {

const AmigaPalette MIRAGE_PALETTE = {
    0x505, 0xC8B, 0x435, 0x546, 0x747, 0x858, 0x969, 0xB7A, 0x402, 0xD9C, 0xDAD,
    0xECE, 0xFEF, 0xFFF, 0xAEE, 0x000, 0x000, 0xEC8, 0xC60, 0xEA0, 0x27F, 0x49D,
    0x5AE, 0xADF, 0xBDF, 0xCEF, 0xFFF, 0x408, 0xA0E, 0xE0E, 0xE08, 0xEEE};

constexpr FotoSequence::Timings MIRAGE_TIMINGS{5, 200, 5, 70};

bool run(FotoSequence &sequence, int frames) {
  bool changed = false;
  for (int frame = 0; frame < frames; ++frame) {
    changed = sequence.advance() || changed;
  }
  return changed;
}

} // namespace

SCENARIO("FotoSequence plays the Mirage logo as state_02 does") {
  GIVEN("The Mirage logo's palette and timings") {
    FotoSequence sequence(MIRAGE_PALETTE, MIRAGE_TIMINGS);

    THEN("The picture starts out all white") {
      REQUIRE(sequence.palette() == AmigaPalette(32, 0xFFF));
    }

    WHEN("FOTO's Wait 5 passes") {
      const bool changed = run(sequence, 5);

      THEN("It is still all white") {
        REQUIRE_FALSE(changed);
        REQUIRE(sequence.palette() == AmigaPalette(32, 0xFFF));
      }

      AND_WHEN("The next frame comes") {
        const bool stepped = sequence.advance();

        THEN("The fade from white takes its first step") {
          REQUIRE(stepped);
          REQUIRE(sequence.palette()[0] == 0xEEE);
          REQUIRE(sequence.palette()[13] == 0xFFF);
        }
      }
    }

    WHEN("FOTO's Wait 75 is over") {
      run(sequence, 5 + 75);

      THEN("The logo shows in its own colours") {
        REQUIRE(sequence.palette() == MIRAGE_PALETTE);
      }

      AND_WHEN("The Wait 200 passes") {
        const bool changed = run(sequence, 200);

        THEN("Nothing changes while the logo stays up") {
          REQUIRE_FALSE(changed);
          REQUIRE(sequence.palette() == MIRAGE_PALETTE);
          REQUIRE_FALSE(sequence.isFinished());
        }
      }
    }

    WHEN("The Fade 5 has run for its Wait 70") {
      run(sequence, 5 + 75 + 200 + 70);

      THEN("The screen closes after 350 frames, seven seconds at 50 Hz") {
        REQUIRE(sequence.isFinished());
      }

      THEN("Wait 70 cuts the fade one step short of black") {
        REQUIRE(sequence.palette()[13] == 0x111);
        REQUIRE(sequence.palette()[12] == 0x101);
        REQUIRE(sequence.palette()[0] == 0x000);
      }

      THEN("Nothing happens after the end") {
        const auto last = sequence.palette();
        REQUIRE_FALSE(sequence.advance());
        REQUIRE(sequence.palette() == last);
      }
    }

    WHEN("One frame less has passed") {
      run(sequence, 5 + 75 + 200 + 70 - 1);

      THEN("The screen is still up") { REQUIRE_FALSE(sequence.isFinished()); }
    }
  }

  GIVEN("A sequence that waits long enough for the fade out to finish") {
    FotoSequence sequence(MIRAGE_PALETTE, {5, 200, 5, 75});

    WHEN("It has run to its end") {
      run(sequence, 5 + 75 + 200 + 75);

      THEN("It ends completely black") {
        REQUIRE(sequence.isFinished());
        REQUIRE(sequence.palette() == AmigaPalette(32, 0x000));
      }
    }
  }
}
