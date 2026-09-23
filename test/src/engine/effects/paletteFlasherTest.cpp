#include "../../../../src/engine/effects/PaletteFlasher.h"
#include <catch2/catch_all.hpp>
#include <stdexcept>
#include <vector>

using namespace openfranko::src::engine::effects;

SCENARIO("PaletteFlasher cycles a colour like the AMOS Flash instruction") {
  GIVEN("Colour 1 flashing $F00 for 4 frames and $800 for 2") {
    AmigaPalette palette = {0x004, 0xF55, 0x999};
    PaletteFlasher flasher;
    flasher.start(1, {{0xF00, 4}, {0x800, 2}});

    WHEN("The next frame comes") {
      const bool changed = flasher.tick(palette);

      THEN("The first colour shows at once and no other colour changes") {
        REQUIRE(changed);
        REQUIRE(palette == AmigaPalette{0x004, 0xF00, 0x999});
      }
    }

    WHEN("Frames keep coming") {
      std::vector<AmigaColor> shown;
      std::vector<int> changeFrames;
      for (int frame = 1; frame <= 13; ++frame) {
        if (flasher.tick(palette)) {
          changeFrames.push_back(frame);
        }
        shown.push_back(palette[1]);
      }

      THEN("Each colour stays for its own frames and the list starts over") {
        REQUIRE(shown == std::vector<AmigaColor>{
                             0xF00, 0xF00, 0xF00, 0xF00, 0x800, 0x800, 0xF00,
                             0xF00, 0xF00, 0xF00, 0x800, 0x800, 0xF00});
      }

      THEN("It reports a change only on the frames the colour changes") {
        REQUIRE(changeFrames == std::vector<int>{1, 5, 7, 11, 13});
      }
    }
  }

  GIVEN("A flash that is given an empty list") {
    AmigaPalette palette = {0x123};
    PaletteFlasher flasher;
    flasher.start(0, {{0xF00, 1}});
    flasher.start(0, {});

    THEN("The colour stops flashing, as with Flash n,\"\"") {
      REQUIRE_FALSE(flasher.isFlashing());
      REQUIRE_FALSE(flasher.tick(palette));
      REQUIRE(palette == AmigaPalette{0x123});
    }
  }

  GIVEN("A flash of a colour past the end of the palette") {
    AmigaPalette palette = {0x000};
    PaletteFlasher flasher;
    flasher.start(1, {{0xF00, 1}});

    THEN("It refuses to write outside the palette") {
      REQUIRE_THROWS_AS(flasher.tick(palette), std::out_of_range);
    }
  }

  GIVEN("Flash lists AMOS refuses") {
    PaletteFlasher flasher;

    THEN("A colour that stays for less than a frame is refused") {
      REQUIRE_THROWS_AS(flasher.start(0, {{0xF00, 0}}), std::invalid_argument);
    }

    THEN("More than 16 colours are refused, 16 are not") {
      REQUIRE_THROWS_AS(flasher.start(0, FlashSteps(17, FlashStep{0xF00, 1})),
                        std::invalid_argument);
      REQUIRE_NOTHROW(flasher.start(0, FlashSteps(16, FlashStep{0xF00, 1})));
    }
  }
}
