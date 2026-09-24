#include "../../../../src/engine/effects/InkeyBuffer.h"
#include <catch2/catch_all.hpp>

#include <string>

using namespace openfranko::src::engine::effects;

namespace {

void pressAll(InkeyBuffer &keyboard, const std::string &keys) {
  for (const char key : keys) {
    keyboard.press(key);
  }
}

std::string readAll(InkeyBuffer &keyboard) {
  std::string read;
  while (const auto key = keyboard.inkey()) {
    read += *key;
  }
  return read;
}

} // namespace

SCENARIO("InkeyBuffer holds keys back under Forbid as input.device does") {
  GIVEN("A keyboard under Forbid") {
    InkeyBuffer keyboard;

    WHEN("Keys are pressed") {
      pressAll(keyboard, "CE");

      THEN("Inkey$ gets nothing") {
        REQUIRE(keyboard.isEmpty());
        REQUIRE_FALSE(keyboard.inkey());
      }

      AND_WHEN("BASIC sleeps in a Wait") {
        keyboard.sleep();

        THEN("They arrive in order") { REQUIRE(readAll(keyboard) == "CE"); }

        AND_WHEN("More keys are pressed afterwards") {
          pressAll(keyboard, "NT");

          THEN("Forbid holds them back again") {
            REQUIRE(readAll(keyboard) == "CE");
          }
        }
      }

      AND_WHEN("Permit is called") {
        keyboard.permit();
        pressAll(keyboard, "NT");

        THEN("They arrive, and later keys come straight through") {
          REQUIRE(readAll(keyboard) == "CENT");
        }
      }
    }

    WHEN("More keys than AMOS's buffer holds arrive at once") {
      std::string keys;
      for (int i = 0; i < 40; ++i) {
        keys += static_cast<char>('A' + i % 26);
      }
      pressAll(keyboard, keys);
      keyboard.sleep();

      THEN("The first 31 are kept and the rest are lost") {
        REQUIRE(readAll(keyboard) == keys.substr(0, InkeyBuffer::CAPACITY));
      }
    }
  }
}
