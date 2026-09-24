#include "../../../../src/engine/street/CheatCodes.h"
#include <catch2/catch_all.hpp>

#include <string>

using namespace openfranko::src::engine::street;

namespace {

constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RN = 13;
constexpr int RO = 14;

std::string typed(const std::string &keys, std::string text = "") {
  for (const char key : keys) {
    typeCheatKey(text, key);
  }
  return text;
}

GameSession afterTyping(const std::string &keys) {
  GameSession session;
  session.textBuffer = typed(keys, session.textBuffer);
  applyCheatCodes(session);
  return session;
}

} // namespace

SCENARIO("A fresh session is what boot and state 05 leave for the menu") {
  GIVEN("A new session") {
    const GameSession session;

    THEN("Full energy, three lives, no stage and no kills") {
      REQUIRE(session.registers[RF] == 64);
      REQUIRE(session.registers[RG] == 3);
      REQUIRE(session.registers[RO] == -1);
      REQUIRE(session.registers[RN] == 0);
    }

    THEN("N$ still holds the hi-score file name and no cheat is on") {
      REQUIRE(session.textBuffer == HighScoreTable::FILE_NAME);
      REQUIRE_FALSE(session.shortLevels);
      REQUIRE_FALSE(session.brutality);
    }
  }
}

SCENARIO("typeCheatKey adds a key to N$ as state 07 does") {
  GIVEN("N$ left by a hi-score name") {
    const std::string name = "MARIUSZ        ";

    WHEN("doman is typed") {
      const std::string text = typed("doman", name);

      THEN("Each key is upper-cased and shifted down by 4, and only the last "
           "15 characters stay") {
        REQUIRE(text == "SZ        @KI=J");
      }
    }
  }
}

SCENARIO("applyCheatCodes decodes N$ as the end of state 07 does") {
  GIVEN("Nothing typed") {
    const GameSession session = afterTyping("");

    THEN("Nothing changes") {
      REQUIRE(session.registers[RG] == 3);
      REQUIRE(session.registers[RO] == -1);
      REQUIRE_FALSE(session.shortLevels);
      REQUIRE_FALSE(session.brutality);
    }
  }

  GIVEN("Each code typed on its own") {
    THEN("TAVRIA, MUTANT and DOMAN set the lives") {
      REQUIRE(afterTyping("TAVRIA").registers[RG] == 10000);
      REQUIRE(afterTyping("MUTANT").registers[RG] == 15);
      REQUIRE(afterTyping("DOMAN").registers[RG] == 9);
    }

    THEN("CENT and DRZE start on stage 2 and 3") {
      REQUIRE(afterTyping("CENT").registers[RO] == 1);
      REQUIRE(afterTyping("DRZE").registers[RO] == 2);
    }

    THEN("SKIP shortens the levels and MORAL turns the gore on") {
      REQUIRE(afterTyping("SKIP").shortLevels);
      REQUIRE(afterTyping("MORAL").brutality);
    }

    THEN("A code typed in lower case or among other keys still counts") {
      REQUIRE(afterTyping("x doman 1").registers[RG] == 9);
    }
  }

  GIVEN("Two codes of a kind") {
    THEN("The one tested last wins, whatever the typing order") {
      REQUIRE(afterTyping("DOMANTAVRIA").registers[RG] == 9);
      REQUIRE(afterTyping("DRZECENT").registers[RO] == 2);
    }
  }

  GIVEN("A code pushed partly out of the 15 characters") {
    THEN("It no longer counts") {
      REQUIRE(afterTyping("DOMAN0123456789").registers[RG] == 9);
      REQUIRE(afterTyping("DOMAN0123456789A").registers[RG] == 3);
    }
  }

  GIVEN("A session that had SKIP and MORAL") {
    GameSession session = afterTyping("SKIPMORAL");
    session.textBuffer = typed("CENT", session.textBuffer);
    session.textBuffer = typed("0123456789A", session.textBuffer);

    WHEN("The menu is left without them") {
      applyCheatCodes(session);

      THEN("SKIP is decided again but MORAL stays on") {
        REQUIRE_FALSE(session.shortLevels);
        REQUIRE(session.brutality);
      }
    }
  }

  GIVEN("N$ written by the other parts of the game") {
    THEN("A hi-score name holding OGEL turns SKIP on") {
      GameSession session;
      session.textBuffer = "VOGEL          ";
      applyCheatCodes(session);
      REQUIRE(session.shortLevels);
    }

    THEN("The password the bonus drive shows is not the code itself") {
      GameSession session;
      session.textBuffer = "KOD: CENT";
      applyCheatCodes(session);
      REQUIRE(session.registers[RO] == -1);
    }
  }
}
