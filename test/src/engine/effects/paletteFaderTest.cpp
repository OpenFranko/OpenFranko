#include "../../../../src/engine/effects/PaletteFader.h"
#include <catch2/catch_all.hpp>
#include <stdexcept>
#include <vector>

using namespace openfranko::src::engine::effects;

SCENARIO("PaletteFader steps like the AMOS Fade instruction") {
  GIVEN("A white colour fading to $505 at speed 5") {
    AmigaPalette palette = {0xFFF};
    PaletteFader fader;
    fader.start(palette, 5, {0x505});

    WHEN("The next frame comes") {
      const bool changed = fader.tick(palette);

      THEN("The first step lands at once, every nibble one unit closer") {
        REQUIRE(changed);
        REQUIRE(palette[0] == 0xEEE);
      }
    }

    WHEN("Frames keep coming") {
      std::vector<int> stepFrames;
      AmigaColor afterTenSteps = 0;
      for (int frame = 1; frame <= 100; ++frame) {
        if (fader.tick(palette)) {
          stepFrames.push_back(frame);
        }
        if (frame == 46) {
          afterTenSteps = palette[0];
        }
      }

      THEN("Each channel moves on its own: after 10 steps it is $555") {
        REQUIRE(afterTenSteps == 0x555);
      }

      THEN("Every later step comes 5 frames after the previous one") {
        REQUIRE(stepFrames.at(0) == 1);
        REQUIRE(stepFrames.at(1) == 6);
        REQUIRE(stepFrames.at(2) == 11);
      }

      THEN("It takes 15 steps, the last on frame 1 + 14 * 5") {
        REQUIRE(stepFrames.size() == 15);
        REQUIRE(stepFrames.back() == 71);
      }

      THEN("It stops exactly on the target") {
        REQUIRE(palette[0] == 0x505);
        REQUIRE_FALSE(fader.isFading());
      }
    }
  }

  GIVEN("A fade that lists only some colours") {
    AmigaPalette palette = {0x111, 0x222, 0x333};
    PaletteFader fader;
    fader.start(palette, 1, {PaletteFader::KEEP, 0x000});

    WHEN("It runs to the end") {
      while (fader.isFading()) {
        fader.tick(palette);
      }

      THEN("A KEEP entry and the entries past the target are untouched") {
        REQUIRE(palette == AmigaPalette{0x111, 0x000, 0x333});
      }
    }
  }

  GIVEN("A fade whose target is already reached") {
    AmigaPalette palette = {0x000, 0x7A3};
    PaletteFader fader;
    fader.start(palette, 1, {0x000, 0x7A3});

    THEN("There is nothing to do and the palette does not change") {
      REQUIRE_FALSE(fader.isFading());
      REQUIRE_FALSE(fader.tick(palette));
      REQUIRE(palette == AmigaPalette{0x000, 0x7A3});
    }
  }

  GIVEN("A slow fade to black that is replaced after one step") {
    AmigaPalette palette = {0xFFF, 0x505};
    PaletteFader fader;
    fader.start(palette, 100, {0x000, 0x000});
    fader.tick(palette);
    fader.start(palette, 1, palette);

    WHEN("More frames pass") {
      bool changed = false;
      for (int frame = 0; frame < 200; ++frame) {
        changed = fader.tick(palette) || changed;
      }

      THEN("The palette stays one step darker for good") {
        REQUIRE_FALSE(changed);
        REQUIRE(palette == AmigaPalette{0xEEE, 0x404});
      }
    }
  }

  GIVEN("A speed below 1") {
    AmigaPalette palette = {0xFFF};
    PaletteFader fader;

    THEN("Starting the fade is refused, as AMOS refuses it") {
      REQUIRE_THROWS_AS(fader.start(palette, 0, {0x000}),
                        std::invalid_argument);
    }
  }
}
