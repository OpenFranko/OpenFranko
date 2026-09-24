#include "../../../../src/engine/effects/CodeCardCheck.h"
#include <array>
#include <catch2/catch_all.hpp>
#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace openfranko::src::engine::effects;

namespace {

std::vector<uint8_t> makeCards() {
  std::vector<uint8_t> cards(200, 3);
  cards[2 + 10 * 5] = 7;
  cards[100 + 9 + 10 * 0] = 10;
  return cards;
}

constexpr CodeCardCheck::Cell CARD_1_CELL{2, 5};
constexpr CodeCardCheck::Cell CARD_2_CELL{9, 0};

} // namespace

SCENARIO("CodeCardCheck asks one cell of each card, as CHECK[1] does") {
  GIVEN("Card 1 holds H at (2,5) and card 2 holds K at (9,0)") {
    CodeCardCheck check(makeCards(), {CARD_1_CELL, CARD_2_CELL});

    THEN("It asks about card 1 first") {
      REQUIRE(check.cell().x == 2);
      REQUIRE(check.cell().y == 5);
      REQUIRE_FALSE(check.isFinished());
    }

    WHEN("Both answers are right") {
      check.answer('H');
      check.answer('K');

      THEN("The check is passed") {
        REQUIRE(check.isFinished());
        REQUIRE(check.isPassed());
      }
    }

    WHEN("The first answer is wrong") {
      check.answer('D');

      THEN("Card 2 is still asked, with no second try on card 1") {
        REQUIRE_FALSE(check.isFinished());
        REQUIRE(check.cell().x == 9);
        REQUIRE(check.cell().y == 0);
      }

      AND_WHEN("The second answer is right") {
        check.answer('K');

        THEN("The check is failed") {
          REQUIRE(check.isFinished());
          REQUIRE_FALSE(check.isPassed());
        }
      }
    }

    WHEN("Only the second answer is wrong") {
      check.answer('H');
      check.answer('A');

      THEN("The check is failed") {
        REQUIRE(check.isFinished());
        REQUIRE_FALSE(check.isPassed());
      }
    }

    WHEN("Keys outside A to K are typed") {
      check.answer('L');
      check.answer('Z');
      check.answer('@');
      check.answer('h');

      THEN("They are ignored") {
        REQUIRE_FALSE(check.isFinished());
        REQUIRE(check.cell().x == 2);
        REQUIRE(check.cell().y == 5);
      }
    }

    WHEN("A letter comes after the check is over") {
      check.answer('H');
      check.answer('K');
      check.answer('A');

      THEN("It changes nothing") { REQUIRE(check.isPassed()); }
    }
  }

  GIVEN("Card data of the wrong size") {
    THEN("The check refuses it") {
      REQUIRE_THROWS_AS(CodeCardCheck(std::vector<uint8_t>(199, 0),
                                      {CARD_1_CELL, CARD_2_CELL}),
                        std::invalid_argument);
    }
  }

  GIVEN("A cell off the card") {
    THEN("The check refuses it") {
      REQUIRE_THROWS_AS(CodeCardCheck(makeCards(), {CARD_1_CELL, {10, 0}}),
                        std::invalid_argument);
    }
  }
}

SCENARIO("The stage 3 check gives card 1 three tries, as CHECK[2] does") {
  GIVEN("Card 1 holds H at (2,5) and D everywhere else") {
    const std::array<CodeCardCheck::Cell, CodeCardCheck::STAGE_TRIES> tries = {
        {{2, 5}, {9, 0}, {0, 0}}};
    CodeCardCheck check = CodeCardCheck::stageCheck(makeCards(), tries);

    WHEN("The first answer is right") {
      check.answer('H');

      THEN("It is passed at once") {
        REQUIRE(check.isFinished());
        REQUIRE(check.isPassed());
      }
    }

    WHEN("The first answer is wrong") {
      check.answer('A');

      THEN("A new cell of card 1 is asked, not card 2") {
        REQUIRE_FALSE(check.isFinished());
        REQUIRE(check.cell().x == 9);
        REQUIRE(check.cell().y == 0);
      }

      AND_WHEN("Card 1's letter at (9,0) is given, not card 2's") {
        check.answer('D');

        THEN("It is passed on the second try") { REQUIRE(check.isPassed()); }
      }
    }

    WHEN("All three answers are wrong") {
      check.answer('A');
      check.answer('K');
      check.answer('B');

      THEN("It is failed") {
        REQUIRE(check.isFinished());
        REQUIRE_FALSE(check.isPassed());
      }
    }
  }
}
