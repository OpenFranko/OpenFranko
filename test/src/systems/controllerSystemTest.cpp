#include "../../../src/systems/ControllerSystem.h"
#include <catch2/catch_all.hpp>

using namespace openfranko::src::systems;

namespace {

KeyEvent event(Key key, int code, char character, bool pressed,
               bool repeat = false) {
  KeyEvent keyEvent;
  keyEvent.key = key;
  keyEvent.code = code;
  keyEvent.character = character;
  keyEvent.pressed = pressed;
  keyEvent.repeat = repeat;
  return keyEvent;
}

void press(ControllerSystem &controller, Key key, int code = 0,
           char character = 0) {
  controller.receiveKey(event(key, code, character, true));
}

void release(ControllerSystem &controller, Key key, int code = 0,
             char character = 0) {
  controller.receiveKey(event(key, code, character, false));
}

void repeat(ControllerSystem &controller, Key key, int code, char character) {
  controller.receiveKey(event(key, code, character, true, true));
}

} // namespace

SCENARIO("The keyboard stands in for the joystick") {
  GIVEN("A controller on the front end") {
    ControllerSystem controller;
    controller.setKeyMode(KeyMode::FrontEnd);

    WHEN("Right is held") {
      press(controller, Key::Right, 1);
      controller.update();

      THEN("The joystick reads right") {
        REQUIRE(controller.states.right);
        REQUIRE(controller.joystick() == 8);
      }

      AND_WHEN("It is let go") {
        release(controller, Key::Right, 1);
        controller.update();

        THEN("The joystick is centred") { REQUIRE(controller.joystick() == 0); }
      }
    }

    WHEN("Left and Right are held together") {
      press(controller, Key::Left, 1);
      press(controller, Key::Right, 2);
      controller.update();

      THEN("They cancel") { REQUIRE(controller.joystick() == 0); }
    }

    WHEN("D is held") {
      press(controller, Key::D, 7, 'd');
      controller.update();

      THEN("It is a letter, not the joystick") {
        REQUIRE_FALSE(controller.states.right);
      }

      AND_WHEN("The street starts") {
        controller.setKeyMode(KeyMode::Game);
        controller.update();

        THEN("It steers") { REQUIRE(controller.states.right); }
      }
    }
  }
}

SCENARIO("Space is the fire button") {
  GIVEN("A controller on the front end") {
    ControllerSystem controller;

    WHEN("Space is pressed with no direction") {
      press(controller, Key::Space, 3, ' ');
      controller.update();

      THEN("Fire is down and latched") {
        REQUIRE(controller.states.button);
        REQUIRE(controller.joystick() == 16);
        REQUIRE(controller.isFireLatched());
      }

      AND_WHEN("The latch is cleared") {
        release(controller, Key::Space, 3, ' ');
        controller.update();
        controller.clearFireLatch();

        THEN("It stays clear") { REQUIRE_FALSE(controller.isFireLatched()); }
      }
    }

    WHEN("Space is pressed while Up is held") {
      press(controller, Key::Up, 1);
      press(controller, Key::Space, 3, ' ');
      controller.update();

      THEN("Fire is down but not latched") {
        REQUIRE(controller.states.button);
        REQUIRE_FALSE(controller.isFireLatched());
      }
    }
  }

  GIVEN("A controller in the name entry") {
    ControllerSystem controller;
    controller.setKeyMode(KeyMode::NameEntry);

    THEN("Space is no fire button") {
      press(controller, Key::Space, 3, ' ');
      controller.update();
      REQUIRE_FALSE(controller.states.button);
    }
  }
}

SCENARIO("Typed keys depend on the mode") {
  GIVEN("A controller on the front end") {
    ControllerSystem controller;

    WHEN("A letter, Space and Return are typed") {
      press(controller, Key::A, 4, 'a');
      press(controller, Key::Space, 3, ' ');
      press(controller, Key::Other, 5, '\r');
      controller.update();

      THEN("The letter and Return are typed, but Space is fire") {
        REQUIRE(controller.typedKeys() == "a\r");
      }

      AND_WHEN("The next update comes") {
        controller.update();

        THEN("They are gone") { REQUIRE(controller.typedKeys().empty()); }
      }
    }
  }

  GIVEN("A controller in the name entry") {
    ControllerSystem controller;
    controller.setKeyMode(KeyMode::NameEntry);

    THEN("Space types a space and Backspace a backspace") {
      press(controller, Key::Space, 3, ' ');
      press(controller, Key::Other, 6, '\b');
      controller.update();
      REQUIRE(controller.typedKeys() == " \b");
    }
  }

  GIVEN("A controller in the street") {
    ControllerSystem controller;
    controller.setKeyMode(KeyMode::Game);

    THEN("Nothing is typed") {
      press(controller, Key::Other, 8, 'q');
      controller.update();
      REQUIRE(controller.typedKeys().empty());
    }
  }
}

SCENARIO("A held key repeats only if it was pressed in a typing mode") {
  GIVEN("Q pressed in the street and E pressed on the front end") {
    ControllerSystem controller;
    controller.setKeyMode(KeyMode::Game);
    press(controller, Key::Other, 8, 'q');
    controller.update();
    controller.setKeyMode(KeyMode::FrontEnd);
    press(controller, Key::Other, 9, 'e');
    controller.update();

    WHEN("Both repeat") {
      repeat(controller, Key::Other, 8, 'q');
      repeat(controller, Key::Other, 9, 'e');
      repeat(controller, Key::Other, 9, 'e');
      controller.update();

      THEN("Only E types") { REQUIRE(controller.typedKeys() == "ee"); }
    }
  }
}

SCENARIO("The key register holds the frame's last key event") {
  GIVEN("A controller in the street") {
    ControllerSystem controller;
    controller.setKeyMode(KeyMode::Game);

    THEN("A key's press and its release both reach it") {
      press(controller, Key::Other, 10, 'p');
      controller.update();
      REQUIRE(controller.functionKey() == FunctionKey::Other);
      release(controller, Key::Other, 10, 'p');
      controller.update();
      REQUIRE(controller.functionKey() == FunctionKey::Other);
    }

    THEN("The function keys and Esc read as themselves") {
      press(controller, Key::F2, 11);
      controller.update();
      REQUIRE(controller.functionKey() == FunctionKey::F2);
      press(controller, Key::Escape, 12);
      controller.update();
      REQUIRE(controller.functionKey() == FunctionKey::Escape);
    }

    THEN("The last event of a frame wins, and it lasts one update") {
      press(controller, Key::F3, 13);
      release(controller, Key::Other, 10, 'p');
      controller.update();
      REQUIRE(controller.functionKey() == FunctionKey::Other);
      controller.update();
      REQUIRE_FALSE(controller.functionKey());
    }

    THEN("The joystick keys and repeats never touch it") {
      press(controller, Key::D, 7, 'd');
      press(controller, Key::Space, 3, ' ');
      repeat(controller, Key::F1, 14, 0);
      controller.update();
      REQUIRE_FALSE(controller.functionKey());
    }
  }

  GIVEN("A controller on the front end") {
    ControllerSystem controller;

    THEN("D is an ordinary key there") {
      press(controller, Key::D, 7, 'd');
      controller.update();
      REQUIRE(controller.functionKey() == FunctionKey::Other);
    }
  }
}

SCENARIO("Delete and the left mouse button are read at each update") {
  GIVEN("A controller") {
    ControllerSystem controller;

    WHEN("Both are held") {
      press(controller, Key::Delete, 15);
      controller.receiveMouseButton(true);

      THEN("They show only from the next update") {
        REQUIRE_FALSE(controller.isDeleteHeld());
        REQUIRE_FALSE(controller.isMouseButtonDown());
        controller.update();
        REQUIRE(controller.isDeleteHeld());
        REQUIRE(controller.isMouseButtonDown());
      }

      AND_WHEN("Both are let go") {
        controller.update();
        release(controller, Key::Delete, 15);
        controller.receiveMouseButton(false);
        controller.update();

        THEN("Neither is held") {
          REQUIRE_FALSE(controller.isDeleteHeld());
          REQUIRE_FALSE(controller.isMouseButtonDown());
        }
      }
    }
  }
}
