#include "../../../../src/engine/effects/CharacterSelection.h"
#include <catch2/catch_all.hpp>

using namespace openfranko::src::engine::effects;

namespace {

using Joystick = CharacterSelection::Joystick;

const Joystick NOTHING{};
const Joystick LEFT{true, false, false};
const Joystick RIGHT{false, true, false};
const Joystick FIRE{false, false, true};

void run(CharacterSelection &selection, int frames,
         const Joystick &joystick = NOTHING) {
  for (int frame = 0; frame < frames; ++frame) {
    selection.advance(joystick);
  }
}

} // namespace

SCENARIO("CharacterSelection chooses between Franko and Alex as TWARZ does") {
  GIVEN("A new selection") {
    GameOptions options;
    options.character = Character::Alex;
    CharacterSelection selection(options);

    THEN("The hand points at Franko, and Franko is chosen") {
      REQUIRE(options.character == Character::Franko);
      REQUIRE(selection.hand().shown);
      REQUIRE(selection.hand().x == 64);
      REQUIRE(selection.hand().y == 150);
      REQUIRE(selection.hand().image == 1);
      REQUIRE_FALSE(selection.hand().flipped);
      REQUIRE_FALSE(selection.face().shown);
    }

    WHEN("The joystick goes right") {
      selection.advance(RIGHT);

      THEN("The hand turns round to point at Alex") {
        REQUIRE(options.character == Character::Alex);
        REQUIRE(selection.hand().x == 280);
        REQUIRE(selection.hand().flipped);
      }

      AND_WHEN("It goes left again") {
        selection.advance(LEFT);

        THEN("Franko is chosen again") {
          REQUIRE(options.character == Character::Franko);
          REQUIRE(selection.hand().x == 64);
        }
      }
    }
  }
}

SCENARIO("CharacterSelection plays the confirmation as TWARZ does") {
  GIVEN("Franko confirmed with the music option off") {
    GameOptions options;
    options.music = false;
    CharacterSelection selection(options);
    selection.advance(FIRE);

    THEN("The hand starts to waggle at once") {
      REQUIRE(selection.hand().x == 66);
    }

    WHEN("The joystick goes right after fire") {
      selection.advance(RIGHT);

      THEN("The choice no longer changes") {
        REQUIRE(options.character == Character::Franko);
      }
    }

    WHEN("MACH's Wait 40 is over") {
      run(selection, 39);
      const bool sampleBefore = selection.sample().has_value();
      selection.advance(NOTHING);

      THEN("Franko's face bob appears hidden and his voice plays once") {
        REQUIRE_FALSE(sampleBefore);
        REQUIRE(selection.face().shown);
        REQUIRE(selection.face().x == 148);
        REQUIRE(selection.face().y == 117);
        REQUIRE(selection.face().image == 0);
        REQUIRE(selection.sample() == 1);
        selection.advance(NOTHING);
        REQUIRE_FALSE(selection.sample().has_value());
      }
    }

    WHEN("The face blinks") {
      run(selection, 49);
      const int hiddenStill = selection.face().image;
      selection.advance(NOTHING);
      const int shown = selection.face().image;
      run(selection, 10);
      const int hiddenAgain = selection.face().image;
      run(selection, 10);

      THEN("It shows 10 frames after the start, then every 10 frames") {
        REQUIRE(hiddenStill == 0);
        REQUIRE(shown == 2);
        REQUIRE(hiddenAgain == 0);
        REQUIRE(selection.face().image == 2);
      }
    }

    WHEN("The Wait 50 is over") {
      run(selection, 89);
      const bool finishedBefore = selection.isFinished();
      selection.advance(NOTHING);

      THEN("The bobs go, the music stops without a fade and the state ends") {
        REQUIRE_FALSE(finishedBefore);
        REQUIRE_FALSE(selection.hand().shown);
        REQUIRE_FALSE(selection.face().shown);
        REQUIRE(selection.stopsMusic());
        REQUIRE_FALSE(selection.musicVolume().has_value());
        REQUIRE(selection.isFinished());
      }
    }
  }

  GIVEN("Alex confirmed with the music option on") {
    GameOptions options;
    options.music = true;
    CharacterSelection selection(options);
    selection.advance(RIGHT);
    selection.advance(FIRE);

    WHEN("MACH's Wait 40 is over") {
      run(selection, 40);

      THEN("Alex's voice plays and his face bob sits over him") {
        REQUIRE(selection.sample() == 2);
        REQUIRE(selection.face().x == 189);
        run(selection, 9);
        REQUIRE(selection.face().image == 0);
        run(selection, 1);
        REQUIRE(selection.face().image == 3);
      }
    }

    WHEN("The Wait 50 is over") {
      run(selection, 90);

      THEN("SCICH starts at full volume, with the bobs gone") {
        REQUIRE(selection.musicVolume() == 63);
        REQUIRE_FALSE(selection.hand().shown);
        REQUIRE_FALSE(selection.stopsMusic());
        REQUIRE_FALSE(selection.isFinished());
      }

      AND_WHEN("The fade runs") {
        run(selection, 63);

        THEN("It steps down one level a frame to silence") {
          REQUIRE(selection.musicVolume() == 0);
          REQUIRE_FALSE(selection.isFinished());
        }

        AND_WHEN("One more frame passes") {
          selection.advance(NOTHING);

          THEN("Music Off, Mvolume 63, and the state ends") {
            REQUIRE(selection.stopsMusic());
            REQUIRE(selection.musicVolume() == 63);
            REQUIRE(selection.isFinished());
          }
        }
      }
    }
  }
}
