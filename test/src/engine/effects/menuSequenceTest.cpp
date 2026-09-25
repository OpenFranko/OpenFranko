#include "../../../../src/engine/effects/MenuSequence.h"
#include <catch2/catch_all.hpp>

#include <string>

using namespace openfranko::src::engine::effects;

namespace {

using Joystick = MenuSequence::Joystick;

const AmigaPalette BACKDROP_PALETTE = {0x000, 0x111, 0x333, 0x444, 0x555, 0x666,
                                       0x06F, 0xF00, 0x0F0, 0x0FF, 0xBBB, 0xFFF,
                                       0x000, 0x760, 0xB95, 0xFC0};

const Joystick NOTHING{};
const Joystick UP{true, false, false, false, false};
const Joystick DOWN{false, true, false, false, false};
const Joystick RIGHT{false, false, false, true, false};
const Joystick FIRE{false, false, false, false, true};

constexpr int UNPACKED = 4;
constexpr int OPENING_FRAMES = UNPACKED + 50;
constexpr int SCREEN_CLOSE = 4;

void run(MenuSequence &menu, int frames, const Joystick &joystick = NOTHING) {
  for (int frame = 0; frame < frames; ++frame) {
    menu.advance(joystick);
  }
}

const MenuSequence::Bob &bob(const MenuSequence &menu, int number) {
  return menu.bobs()[number - 1];
}

std::string type(MenuSequence &menu, InkeyBuffer &keyboard,
                 const std::string &keys) {
  std::string read;
  for (const char key : keys) {
    keyboard.press(key);
    menu.advance(NOTHING);
    read += menu.keysRead();
  }
  return read;
}

std::string readDuring(MenuSequence &menu, int frames,
                       const Joystick &joystick = NOTHING) {
  std::string read;
  for (int frame = 0; frame < frames; ++frame) {
    menu.advance(joystick);
    read += menu.keysRead();
  }
  return read;
}

void clickMouse(MenuSequence &menu) {
  menu.setMouseButton(true);
  menu.advance(NOTHING);
  menu.setMouseButton(false);
}

} // namespace

SCENARIO("MenuSequence opens the menu as state_07 does") {
  GIVEN("A menu with the options boot sets") {
    GameOptions options;
    InkeyBuffer keyboard;
    MenuSequence menu(options, BACKDROP_PALETTE, keyboard);

    THEN("The icons wait off screen and the hand points at START") {
      REQUIRE(bob(menu, 4).x == -64);
      REQUIRE(bob(menu, 7).x == 384);
      REQUIRE(bob(menu, 4).image == 42);
      REQUIRE(bob(menu, 5).image == 45);
      REQUIRE(bob(menu, 6).image == 46);
      REQUIRE(bob(menu, 9).image == 52);
      REQUIRE(bob(menu, 10).x == 128);
      REQUIRE(bob(menu, 10).y == 32);
      REQUIRE(bob(menu, 10).flipped);
      REQUIRE_FALSE(bob(menu, 1).shown);
    }

    THEN("Unpack 6 To 0 and Double Buffer hold the screen back four VBLs") {
      REQUIRE_FALSE(menu.isScreenShown());
      run(menu, UNPACKED);
      REQUIRE_FALSE(menu.isScreenShown());
      REQUIRE(bob(menu, 4).x == -64);
      run(menu, 1);
      REQUIRE(menu.isScreenShown());
      REQUIRE(bob(menu, 4).x > -64);
    }

    WHEN("24 frames pass after the unpack") {
      run(menu, UNPACKED + 24);

      THEN("The first pair has landed and the second is still flying") {
        REQUIRE(bob(menu, 4).x == 32);
        REQUIRE(bob(menu, 7).x == 288);
        REQUIRE(bob(menu, 5).x == 13);
      }
    }

    WHEN("44 frames pass after the unpack") {
      run(menu, UNPACKED + 44);

      THEN("All six icons are in place") {
        for (int icon = 4; icon <= 6; ++icon) {
          REQUIRE(bob(menu, icon).x == 32);
          REQUIRE(bob(menu, icon + 3).x == 288);
        }
      }
    }

    WHEN("The opening's Wait 20 is over") {
      run(menu, OPENING_FRAMES);
      const bool shownBefore = bob(menu, 1).shown;
      run(menu, 1);

      THEN("The three credits appear below the screen") {
        REQUIRE_FALSE(shownBefore);
        REQUIRE(bob(menu, 1).y == 350);
        REQUIRE(bob(menu, 2).y == 490);
        REQUIRE(bob(menu, 3).y == 630);
        REQUIRE(bob(menu, 3).image == 57);
      }
    }
  }
}

SCENARIO("MenuSequence moves the hand and toggles the options") {
  GIVEN("An open menu") {
    GameOptions options;
    InkeyBuffer keyboard;
    MenuSequence menu(options, BACKDROP_PALETTE, keyboard);
    run(menu, OPENING_FRAMES);

    WHEN("The joystick goes right") {
      menu.advance(RIGHT);

      THEN("The hand points at the right column, unflipped") {
        REQUIRE(bob(menu, 10).x == 240);
        REQUIRE(bob(menu, 10).y == 32);
        REQUIRE_FALSE(bob(menu, 10).flipped);
      }

      AND_WHEN("Down is held through the Wait 10") {
        run(menu, 9, DOWN);
        const int rowDuringWait = bob(menu, 10).y;
        menu.advance(DOWN);

        THEN("The hand only moves once the wait is over") {
          REQUIRE(rowDuringWait == 32);
          REQUIRE(bob(menu, 10).y == 102);
        }
      }
    }

    WHEN("The joystick goes up from the top row") {
      menu.advance(UP);

      THEN("The hand wraps to the bottom row") {
        REQUIRE(bob(menu, 10).y == 172);
      }
    }

    WHEN("The music icon is fired") {
      menu.advance(DOWN);
      run(menu, 9);
      menu.advance(FIRE);

      THEN("Music is off, its icon says so and the hand waggles") {
        REQUIRE_FALSE(options.music);
        REQUIRE(bob(menu, 5).image == 44);
        REQUIRE(bob(menu, 10).x == 130);
      }

      AND_WHEN("Fire stays down through MACH's Wait 40") {
        run(menu, 39, FIRE);
        const bool musicDuringWait = options.music;
        menu.advance(FIRE);

        THEN("The icon only toggles again after the wait") {
          REQUIRE_FALSE(musicDuringWait);
          REQUIRE(options.music);
          REQUIRE(bob(menu, 5).image == 45);
        }
      }
    }

    WHEN("The right column's bottom icon is fired") {
      menu.advance(RIGHT);
      run(menu, 9);
      menu.advance(UP);
      run(menu, 9);
      menu.advance(FIRE);

      THEN("The tall screen option is on") {
        REQUIRE(options.tallScreen);
        REQUIRE(bob(menu, 9).image == 53);
        REQUIRE_FALSE(options.ntsc);
      }
    }
  }

  GIVEN("Options changed before the menu opens") {
    GameOptions options;
    options.music = false;
    options.mono = true;
    InkeyBuffer keyboard;
    MenuSequence menu(options, BACKDROP_PALETTE, keyboard);

    THEN("Their icons show them") {
      REQUIRE(bob(menu, 5).image == 44);
      REQUIRE(bob(menu, 7).image == 49);
      REQUIRE(bob(menu, 6).image == 46);
    }
  }
}

SCENARIO("MenuSequence leaves through START") {
  GIVEN("An open menu with START fired") {
    GameOptions options;
    InkeyBuffer keyboard;
    MenuSequence menu(options, BACKDROP_PALETTE, keyboard);
    run(menu, OPENING_FRAMES);
    menu.advance(FIRE);

    THEN("START shows pressed") { REQUIRE(bob(menu, 4).image == 43); }

    WHEN("MACH's Wait 40 is over") {
      run(menu, 40);

      THEN("The first pair starts flying out") {
        REQUIRE(bob(menu, 4).x == 27);
        REQUIRE(bob(menu, 7).x == 294);
        REQUIRE(bob(menu, 5).x == 32);
      }
    }

    WHEN("The fly out and its Wait 20 are over") {
      run(menu, 89);
      const AmigaPalette beforeFade = menu.palette();
      menu.advance(NOTHING);

      THEN("Fade 3 takes its first step") {
        REQUIRE(beforeFade == BACKDROP_PALETTE);
        REQUIRE(menu.palette()[11] == 0xEEE);
        REQUIRE_FALSE(menu.isFinished());
      }
    }

    WHEN("Fade 3 has had its Wait 45") {
      run(menu, 134);
      const bool shownBefore = menu.isScreenShown();
      menu.advance(NOTHING);

      THEN("_CLOSE's copper list drops the black screen two VBLs later and "
           "the menu is done after four") {
        REQUIRE(shownBefore);
        REQUIRE(menu.isScreenShown());
        REQUIRE(menu.palette() == AmigaPalette(16, 0x000));
        run(menu, 2);
        REQUIRE_FALSE(menu.isScreenShown());
        for (const MenuSequence::Bob &shown : menu.bobs()) {
          REQUIRE_FALSE(shown.shown);
        }
        REQUIRE_FALSE(menu.isFinished());
        run(menu, 1);
        REQUIRE_FALSE(menu.isFinished());
        run(menu, 1);
        REQUIRE(menu.isFinished());
      }
    }
  }
}

SCENARIO("MenuSequence asks for the attract screens when left alone") {
  GIVEN("An open menu") {
    GameOptions options;
    InkeyBuffer keyboard;
    MenuSequence menu(options, BACKDROP_PALETTE, keyboard);
    run(menu, OPENING_FRAMES);

    WHEN("300 idle frames pass after it opened") {
      run(menu, 301);

      THEN("Nothing happens yet") { REQUIRE_FALSE(menu.isAttractDue()); }

      AND_WHEN("One more frame passes") {
        const int creditY = bob(menu, 1).y;
        menu.advance(NOTHING);
        menu.advance(NOTHING);

        THEN("The attract screens are due and the menu stays frozen") {
          REQUIRE(menu.isAttractDue());
          REQUIRE(bob(menu, 1).y == creditY);
        }

        AND_WHEN("The attract screens are over") {
          menu.resumeAfterAttract();
          run(menu, SCREEN_CLOSE + 301);
          const bool dueAgainTooEarly = menu.isAttractDue();
          menu.advance(NOTHING);

          THEN("Screen Close 1 holds BASIC two VBLs, then the idle count "
               "starts again from zero") {
            REQUIRE_FALSE(dueAgainTooEarly);
            REQUIRE(menu.isAttractDue());
          }
        }

        AND_WHEN("Screen Close 1 runs after Amal On") {
          menu.resumeAfterAttract();
          const int creditBefore = bob(menu, 1).y;
          run(menu, SCREEN_CLOSE);

          THEN("The credits scroll on while BASIC waits") {
            REQUIRE(bob(menu, 1).y != creditBefore);
          }
        }
      }
    }

    WHEN("The hand is moved now and then") {
      for (int i = 0; i < 5; ++i) {
        run(menu, 200);
        menu.advance(DOWN);
      }

      THEN("The attract screens never come") {
        REQUIRE_FALSE(menu.isAttractDue());
      }
    }
  }
}

SCENARIO(
    "The double-buffered menu screen shows each frame's bobs a VBL later") {
  GIVEN("The first icons flying in") {
    GameOptions options;
    InkeyBuffer keyboard;
    MenuSequence menu(options, BACKDROP_PALETTE, keyboard);
    run(menu, UNPACKED + 5);
    const auto before = menu.bobs();
    run(menu, 1);

    THEN("The screen shows where they were a frame ago") {
      REQUIRE(menu.bobs()[3].x != before[3].x);
      REQUIRE(menu.shownBobs()[3].x == before[3].x);
      REQUIRE(menu.shownBobs()[6].x == before[6].x);
    }
  }
}

SCENARIO("MenuSequence reads typed keys only when the keyboard gets through") {
  GIVEN("Keys that reached AMOS's buffer on an earlier screen") {
    GameOptions options;
    InkeyBuffer keyboard;
    keyboard.permit();
    keyboard.press('C');
    keyboard.press('E');
    MenuSequence menu(options, BACKDROP_PALETTE, keyboard);
    const std::string readInOpening = readDuring(menu, OPENING_FRAMES + 1);
    const std::string typedAfterDisable = type(menu, keyboard, "NT");

    THEN("The first loop pass reads them, but _DISABLE holds back new keys") {
      REQUIRE(readInOpening == "CE");
      REQUIRE(typedAfterDisable.empty());
    }
  }

  GIVEN("Keys typed while the icons fly in") {
    GameOptions options;
    InkeyBuffer keyboard;
    MenuSequence menu(options, BACKDROP_PALETTE, keyboard);
    const std::string readWhileFlying = type(menu, keyboard, "CENT");
    const std::string readInOpening = readDuring(menu, OPENING_FRAMES - 4);
    menu.advance(NOTHING);

    THEN("The opening's Waits let them through for the first loop pass") {
      REQUIRE(readWhileFlying.empty());
      REQUIRE(readInOpening.empty());
      REQUIRE(menu.keysRead() == "CENT");
    }
  }

  GIVEN("Keys typed in the open menu under Forbid") {
    GameOptions options;
    InkeyBuffer keyboard;
    MenuSequence menu(options, BACKDROP_PALETTE, keyboard);
    run(menu, OPENING_FRAMES + 1);
    const std::string readWhileTyping = type(menu, keyboard, "CENT");

    THEN("Inkey$ gets none of them") { REQUIRE(readWhileTyping.empty()); }

    WHEN("The left mouse button is pressed") {
      clickMouse(menu);
      const std::string readOnClick = menu.keysRead();
      const std::string readLater = type(menu, keyboard, "DRZE");

      THEN("_ENABLE lets them through at once and later keys as they come") {
        REQUIRE(readOnClick == "CENT");
        REQUIRE(readLater == "DRZE");
      }
    }

    WHEN("The hand is moved") {
      menu.advance(RIGHT);
      const std::string readDuringWait = readDuring(menu, 9);
      menu.advance(NOTHING);

      THEN("Its Wait 10 lets them through and they are read when it ends") {
        REQUIRE(readDuringWait.empty());
        REQUIRE(menu.keysRead() == "CENT");
      }
    }

    WHEN("The joystick is still held when the hand's Wait 10 ends") {
      menu.advance(RIGHT);
      run(menu, 9, RIGHT);
      menu.advance(RIGHT);

      THEN("The rest of the pass reads one key before the hand moves again") {
        REQUIRE(menu.keysRead() == "C");
      }
    }

    WHEN("START is fired") {
      menu.advance(FIRE);

      THEN("The loop is over and they are never read") {
        REQUIRE(readDuring(menu, 200).empty());
      }
    }
  }

  GIVEN("A menu whose attract screens are due") {
    GameOptions options;
    InkeyBuffer keyboard;
    MenuSequence menu(options, BACKDROP_PALETTE, keyboard);
    run(menu, OPENING_FRAMES + 303);
    REQUIRE(menu.isAttractDue());
    keyboard.press('D');

    WHEN("The key comes while an attract screen sits in a Wait") {
      keyboard.sleep();
      menu.resumeAfterAttract();
      const std::string readDuringClose = readDuring(menu, SCREEN_CLOSE);
      menu.advance(NOTHING);

      THEN("The menu loop reads it after Screen Close 1") {
        REQUIRE(readDuringClose.empty());
        REQUIRE(menu.keysRead() == "D");
      }
    }

    WHEN("The key comes during the attract's Timer loop") {
      menu.resumeAfterAttract();
      menu.advance(NOTHING);

      THEN("Forbid still holds it back") { REQUIRE(menu.keysRead().empty()); }
    }
  }
}

SCENARIO("A key read by the menu restarts the attract timer as Timer=0 does") {
  GIVEN("An open menu with the keyboard let through") {
    GameOptions options;
    InkeyBuffer keyboard;
    MenuSequence menu(options, BACKDROP_PALETTE, keyboard);
    run(menu, OPENING_FRAMES + 1);
    clickMouse(menu);

    WHEN("A key is typed every 200 frames") {
      for (int i = 0; i < 5; ++i) {
        run(menu, 200);
        type(menu, keyboard, "A");
      }

      THEN("The attract screens never come") {
        REQUIRE_FALSE(menu.isAttractDue());
      }
    }
  }
}
