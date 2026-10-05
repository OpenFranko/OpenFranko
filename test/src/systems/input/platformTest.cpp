#include "../../../../src/systems/input/Platform.h"

#include "../HeadlessSdl.h"

#include <SDL2/SDL.h>
#include <catch2/catch_all.hpp>

#include <utility>
#include <vector>

using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::systems;

namespace {

void press(SDL_Scancode scancode, SDL_Keycode symbol,
           Uint16 modifiers = KMOD_NONE) {
  pushKey(SDL_KEYDOWN, scancode, symbol, modifiers);
}

void release(SDL_Scancode scancode, SDL_Keycode symbol,
             Uint16 modifiers = KMOD_NONE) {
  pushKey(SDL_KEYUP, scancode, symbol, modifiers);
}

void repeat(SDL_Scancode scancode, SDL_Keycode symbol) {
  pushKey(SDL_KEYDOWN, scancode, symbol, KMOD_NONE, true);
}

void pushMouseButton(Uint32 type, Uint8 button) {
  SDL_Event event{};
  event.type = type;
  event.button.button = button;
  event.button.state = type == SDL_MOUSEBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
  SDL_PushEvent(&event);
}

struct Keyboard {
  explicit Keyboard(KeyMode mode = KeyMode::FrontEnd) {
    controller.setKeyMode(mode);
  }

  bool poll() {
    const bool open = platform.pollEvents(controller);
    controller.update();
    return open;
  }

  Platform platform;
  ControllerSystem controller;
};

} // namespace

SCENARIO("The arrow keys and Space are the joystick") {
  GIVEN("A platform feeding a controller on the front end") {
    Keyboard keyboard;

    WHEN("Up and Left are pressed") {
      press(SDL_SCANCODE_UP, SDLK_UP);
      press(SDL_SCANCODE_LEFT, SDLK_LEFT);
      REQUIRE(keyboard.poll());

      THEN("The joystick reads up and left") {
        REQUIRE(keyboard.controller.joystick() == (JOY_UP | JOY_LEFT));
      }

      AND_WHEN("Up is let go") {
        release(SDL_SCANCODE_UP, SDLK_UP);
        keyboard.poll();

        THEN("Only left is held") {
          REQUIRE(keyboard.controller.joystick() == JOY_LEFT);
        }
      }
    }

    WHEN("Down and Right are pressed") {
      press(SDL_SCANCODE_DOWN, SDLK_DOWN);
      press(SDL_SCANCODE_RIGHT, SDLK_RIGHT);
      keyboard.poll();

      THEN("The joystick reads down and right") {
        REQUIRE(keyboard.controller.joystick() == (JOY_DOWN | JOY_RIGHT));
      }
    }

    WHEN("Space is pressed") {
      press(SDL_SCANCODE_SPACE, SDLK_SPACE);
      keyboard.poll();

      THEN("It is the fire button and types nothing") {
        REQUIRE(keyboard.controller.joystick() == JOY_FIRE);
        REQUIRE(keyboard.controller.typedKeys().empty());
      }
    }
  }
}

SCENARIO("WASD steers in the game and types elsewhere") {
  GIVEN("A controller in the game") {
    Keyboard keyboard(KeyMode::Game);

    WHEN("W and A are pressed") {
      press(SDL_SCANCODE_W, SDLK_w);
      press(SDL_SCANCODE_A, SDLK_a);
      keyboard.poll();

      THEN("They steer up and left") {
        REQUIRE(keyboard.controller.joystick() == (JOY_UP | JOY_LEFT));
        REQUIRE(keyboard.controller.typedKeys().empty());
      }
    }

    WHEN("S and D are pressed") {
      press(SDL_SCANCODE_S, SDLK_s);
      press(SDL_SCANCODE_D, SDLK_d);
      keyboard.poll();

      THEN("They steer down and right") {
        REQUIRE(keyboard.controller.joystick() == (JOY_DOWN | JOY_RIGHT));
      }
    }
  }

  GIVEN("A controller on the front end") {
    Keyboard keyboard;

    WHEN("W, A, S and D are pressed") {
      press(SDL_SCANCODE_W, SDLK_w);
      press(SDL_SCANCODE_A, SDLK_a);
      press(SDL_SCANCODE_S, SDLK_s);
      press(SDL_SCANCODE_D, SDLK_d);
      keyboard.poll();

      THEN("They are typed and the joystick stays centred") {
        REQUIRE(keyboard.controller.typedKeys() == "wasd");
        REQUIRE(keyboard.controller.joystick() == 0);
      }
    }
  }
}

SCENARIO("Function keys, Escape, F9 and Delete keep their identity") {
  GIVEN("A controller on the front end") {
    Keyboard keyboard;

    THEN("F1 to F4 and Escape arrive as the game's function keys") {
      const std::vector<std::pair<SDL_Scancode, FunctionKey>> keys = {
          {SDL_SCANCODE_F1, FunctionKey::F1},
          {SDL_SCANCODE_F2, FunctionKey::F2},
          {SDL_SCANCODE_F3, FunctionKey::F3},
          {SDL_SCANCODE_F4, FunctionKey::F4},
          {SDL_SCANCODE_ESCAPE, FunctionKey::Escape}};
      for (const auto &[scancode, functionKey] : keys) {
        CAPTURE(scancode);
        press(scancode, SDL_SCANCODE_TO_KEYCODE(scancode));
        keyboard.poll();
        REQUIRE(keyboard.controller.functionKey() == functionKey);
        release(scancode, SDL_SCANCODE_TO_KEYCODE(scancode));
        keyboard.poll();
      }
    }

    WHEN("F9 is held") {
      press(SDL_SCANCODE_F9, SDLK_F9);
      keyboard.poll();

      THEN("It is held but is none of the game's function keys") {
        REQUIRE(keyboard.controller.isKeyHeld(Key::F9));
        REQUIRE(keyboard.controller.functionKey() == FunctionKey::Other);
      }
    }

    WHEN("Delete is held") {
      press(SDL_SCANCODE_DELETE, SDLK_DELETE);
      keyboard.poll();

      THEN("Delete is held and nothing is typed") {
        REQUIRE(keyboard.controller.isDeleteHeld());
        REQUIRE(keyboard.controller.typedKeys().empty());
      }

      AND_WHEN("It is let go") {
        release(SDL_SCANCODE_DELETE, SDLK_DELETE);
        keyboard.poll();

        THEN("Delete is no longer held") {
          REQUIRE_FALSE(keyboard.controller.isDeleteHeld());
        }
      }
    }

    WHEN("A key the game does not know is pressed") {
      press(SDL_SCANCODE_Q, SDLK_q);
      keyboard.poll();

      THEN("It is an other key that types its letter") {
        REQUIRE(keyboard.controller.functionKey() == FunctionKey::Other);
        REQUIRE(keyboard.controller.typedKeys() == "q");
      }
    }
  }
}

SCENARIO("Typed characters reach the name entry") {
  GIVEN("A controller in the name entry") {
    Keyboard keyboard(KeyMode::NameEntry);

    WHEN("Printable keys, Space and the editing keys are pressed") {
      press(SDL_SCANCODE_A, SDLK_a);
      press(SDL_SCANCODE_1, SDLK_EXCLAIM);
      press(SDL_SCANCODE_GRAVE, '~');
      press(SDL_SCANCODE_SPACE, SDLK_SPACE);
      press(SDL_SCANCODE_BACKSPACE, SDLK_BACKSPACE);
      press(SDL_SCANCODE_RETURN, SDLK_RETURN);
      press(SDL_SCANCODE_KP_ENTER, SDLK_KP_ENTER);
      keyboard.poll();

      THEN("They are typed in order, both Enter keys as a return") {
        REQUIRE(keyboard.controller.typedKeys() == "a!~ \b\r\r");
      }
    }

    WHEN("Keys outside the printable range are pressed") {
      press(SDL_SCANCODE_TAB, SDLK_TAB);
      press(SDL_SCANCODE_ESCAPE, SDLK_ESCAPE);
      press(SDL_SCANCODE_DELETE, SDLK_DELETE);
      press(SDL_SCANCODE_F1, SDLK_F1);
      press(SDL_SCANCODE_LSHIFT, SDLK_LSHIFT);
      keyboard.poll();

      THEN("Nothing is typed") {
        REQUIRE(keyboard.controller.typedKeys().empty());
      }
    }
  }
}

SCENARIO("A held key repeats its character") {
  GIVEN("A controller in the name entry with B held") {
    Keyboard keyboard(KeyMode::NameEntry);
    press(SDL_SCANCODE_B, SDLK_b);
    keyboard.poll();
    REQUIRE(keyboard.controller.typedKeys() == "b");

    WHEN("The keyboard repeats it twice") {
      repeat(SDL_SCANCODE_B, SDLK_b);
      repeat(SDL_SCANCODE_B, SDLK_b);
      keyboard.poll();

      THEN("It is typed twice more without being a new key press") {
        REQUIRE(keyboard.controller.typedKeys() == "bb");
        REQUIRE_FALSE(keyboard.controller.functionKey());
      }
    }

    WHEN("A repeat arrives for a key that was never pressed") {
      repeat(SDL_SCANCODE_C, SDLK_c);
      keyboard.poll();

      THEN("Nothing is typed") {
        REQUIRE(keyboard.controller.typedKeys().empty());
      }
    }
  }
}

SCENARIO("Alt+Return is kept from the game") {
  GIVEN("A controller in the name entry, where Return types") {
    Keyboard keyboard(KeyMode::NameEntry);

    WHEN("Left Alt+Return is pressed and let go") {
      press(SDL_SCANCODE_RETURN, SDLK_RETURN, KMOD_LALT);
      release(SDL_SCANCODE_RETURN, SDLK_RETURN, KMOD_LALT);
      keyboard.poll();

      THEN("The controller hears nothing") {
        REQUIRE(keyboard.controller.typedKeys().empty());
        REQUIRE_FALSE(keyboard.controller.functionKey());
      }
    }

    WHEN("Right Alt+Return is pressed") {
      press(SDL_SCANCODE_RETURN, SDLK_RETURN, KMOD_RALT);
      keyboard.poll();

      THEN("The controller hears nothing") {
        REQUIRE(keyboard.controller.typedKeys().empty());
        REQUIRE_FALSE(keyboard.controller.functionKey());
      }
    }

    WHEN("Return is pressed alone") {
      press(SDL_SCANCODE_RETURN, SDLK_RETURN);
      keyboard.poll();

      THEN("It types a return") {
        REQUIRE(keyboard.controller.typedKeys() == "\r");
        REQUIRE(keyboard.controller.functionKey() == FunctionKey::Other);
      }
    }

    WHEN("Alt+A is pressed") {
      press(SDL_SCANCODE_A, SDLK_a, KMOD_LALT);
      keyboard.poll();

      THEN("Only Return is held back, the letter is typed") {
        REQUIRE(keyboard.controller.typedKeys() == "a");
      }
    }
  }
}

SCENARIO("The left mouse button is the mouse button") {
  GIVEN("A platform feeding a controller") {
    Keyboard keyboard;

    WHEN("The left button is pressed") {
      pushMouseButton(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT);
      keyboard.poll();

      THEN("The button is down") {
        REQUIRE(keyboard.controller.isMouseButtonDown());
      }

      AND_WHEN("It is let go") {
        pushMouseButton(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT);
        keyboard.poll();

        THEN("The button is up") {
          REQUIRE_FALSE(keyboard.controller.isMouseButtonDown());
        }
      }
    }

    WHEN("The right and middle buttons are pressed") {
      pushMouseButton(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_RIGHT);
      pushMouseButton(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_MIDDLE);
      keyboard.poll();

      THEN("The button stays up") {
        REQUIRE_FALSE(keyboard.controller.isMouseButtonDown());
      }
    }
  }
}

SCENARIO("Closing the window ends the polling") {
  GIVEN("A platform feeding a controller") {
    Keyboard keyboard;

    THEN("An empty queue keeps the window open") { REQUIRE(keyboard.poll()); }

    WHEN("The window is closed") {
      pushEvent(SDL_QUIT);

      THEN("The poll reports it once") {
        REQUIRE_FALSE(keyboard.poll());
        REQUIRE(keyboard.poll());
      }
    }

    WHEN("Events the game does not use arrive") {
      pushEvent(SDL_MOUSEMOTION);
      pushEvent(SDL_USEREVENT);

      THEN("The window stays open and the controller is untouched") {
        REQUIRE(keyboard.poll());
        REQUIRE(keyboard.controller.joystick() == 0);
        REQUIRE(keyboard.controller.typedKeys().empty());
        REQUIRE_FALSE(keyboard.controller.functionKey());
        REQUIRE_FALSE(keyboard.controller.isMouseButtonDown());
      }
    }
  }
}
