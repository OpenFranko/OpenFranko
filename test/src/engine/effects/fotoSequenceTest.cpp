#include "../../../../src/engine/effects/FotoSequence.h"
#include <catch2/catch_all.hpp>
#include <cstddef>
#include <vector>

using namespace openfranko::src::engine::effects;

namespace {

const AmigaPalette MIRAGE_PALETTE = {
    0x505, 0xC8B, 0x435, 0x546, 0x747, 0x858, 0x969, 0xB7A, 0x402, 0xD9C, 0xDAD,
    0xECE, 0xFEF, 0xFFF, 0xAEE, 0x000, 0x000, 0xEC8, 0xC60, 0xEA0, 0x27F, 0x49D,
    0x5AE, 0xADF, 0xBDF, 0xCEF, 0xFFF, 0x408, 0xA0E, 0xE0E, 0xE08, 0xEEE};

constexpr FotoSequence::Timings MIRAGE_TIMINGS{5, 200, 5, 70};

const AmigaPalette SPIDER_PALETTE = {
    0x004, 0x999, 0x666, 0x777, 0x888, 0x999, 0xAAA, 0xBBB, 0xCCC, 0xDDD, 0xBBB,
    0xAAA, 0xCDE, 0xBDE, 0xACD, 0x9BD, 0x8AD, 0x79D, 0x68D, 0x57D, 0x000, 0xFFF,
    0xF55, 0x555, 0x777, 0x888, 0x999, 0xAAA, 0xCCC, 0xDDD, 0x400, 0x300};

constexpr FotoSequence::Timings SPIDER_TIMINGS{5, 200, 5, 75};

constexpr std::size_t EYES = 22;
const FlashSteps EYES_FLASH = {{0xF00, 4}, {0xE00, 4}, {0xD00, 4}, {0xC00, 4},
                               {0xB00, 4}, {0xA00, 4}, {0x900, 4}, {0x800, 4},
                               {0x900, 4}, {0xA00, 4}, {0xB00, 4}, {0xC00, 4},
                               {0xD00, 4}, {0xE00, 4}};

bool run(FotoSequence &sequence, int frames) {
  bool changed = false;
  for (int frame = 0; frame < frames; ++frame) {
    changed = sequence.advance() || changed;
  }
  return changed;
}

std::vector<AmigaColor> runFlashingEyes(FotoSequence &sequence, int frames) {
  std::vector<AmigaColor> eyes;
  for (int frame = 0; frame < frames; ++frame) {
    if (sequence.frame() == sequence.holdStart()) {
      sequence.flash(EYES, EYES_FLASH);
    }
    sequence.advance();
    eyes.push_back(sequence.palette()[EYES]);
  }
  return eyes;
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

SCENARIO("FotoSequence flashes the spider's eyes as state_02 does") {
  GIVEN("The World Software screen, flashing colour 22 once FOTO returns") {
    FotoSequence sequence(SPIDER_PALETTE, SPIDER_TIMINGS);

    THEN("FOTO returns after its Wait 5 and Wait 75") {
      REQUIRE(sequence.holdStart() == 5 + 75);
    }

    WHEN("The picture has been up for the whole Wait 200") {
      runFlashingEyes(sequence, 5 + 75 + 200);

      THEN("Only the eyes differ from the picture's own colours") {
        AmigaPalette expected = SPIDER_PALETTE;
        expected[EYES] = 0x800;
        REQUIRE(sequence.palette() == expected);
      }
    }

    WHEN("The screen has played to its end") {
      const auto eyes = runFlashingEyes(sequence, 5 + 75 + 200 + 75);

      THEN("The eyes fade in with the rest of the picture") {
        REQUIRE(eyes[79] == 0xF55);
      }

      THEN("The flash starts at once and changes colour every 4 frames") {
        REQUIRE(eyes[80] == 0xF00);
        REQUIRE(eyes[83] == 0xF00);
        REQUIRE(eyes[84] == 0xE00);
        REQUIRE(eyes[108] == 0x800);
        REQUIRE(eyes[111] == 0x800);
        REQUIRE(eyes[112] == 0x900);
        REQUIRE(eyes[135] == 0xE00);
        REQUIRE(eyes[136] == 0xF00);
      }

      THEN("The fade wins when it changes the eyes on a flash frame") {
        REQUIRE(eyes[280] == 0x700);
        REQUIRE(eyes[300] == 0x300);
      }

      THEN("Between fade steps the flash puts its own colour back") {
        REQUIRE(eyes[284] == 0xA00);
        REQUIRE(eyes[285] == 0x600);
        REQUIRE(eyes[304] == 0xF00);
      }

      THEN("Once the fade has taken the eyes to black, the flash goes on") {
        REQUIRE(eyes[315] == 0x000);
        REQUIRE(eyes[316] == 0xC00);
        REQUIRE(eyes[320] == 0xB00);
      }

      THEN("The screen closes black except for the eyes") {
        REQUIRE(sequence.isFinished());
        AmigaPalette expected(32, 0x000);
        expected[EYES] = 0xD00;
        REQUIRE(sequence.palette() == expected);
      }
    }
  }
}
