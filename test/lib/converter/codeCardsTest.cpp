#include "../../../lib/converter/codeCards/codeCards.h"
#include <catch2/catch_all.hpp>
#include <string>
#include <vector>

using namespace openfranko::lib::converter::codeCards;

namespace {

std::vector<uint8_t> buildCardData() {
  std::vector<uint8_t> data(10, 0xEE);
  for (size_t i = 0; i < 200; i++) {
    data.push_back(static_cast<uint8_t>(i % 11));
  }
  return data;
}

} // namespace

SCENARIO("parse reads both code cards row by row") {
  GIVEN("Card data after a 10-byte prefix") {
    auto data = buildCardData();

    WHEN("parse is called") {
      auto cards = parse(data);

      THEN("Card 1 starts at byte 10") {
        REQUIRE(cards[0].rows[0][0] == 0);
        REQUIRE(cards[0].rows[0][9] == 9);
        REQUIRE(cards[0].rows[1][0] == 10);
        REQUIRE(cards[0].rows[9][9] == 99 % 11);
      }

      THEN("Card 2 starts at byte 110") {
        REQUIRE(cards[1].rows[0][0] == 100 % 11);
        REQUIRE(cards[1].rows[9][9] == 199 % 11);
      }
    }
  }

  GIVEN("Data one byte short of the end of card 2") {
    auto data = buildCardData();
    data.pop_back();

    WHEN("parse is called") {
      THEN("It throws a runtime_error") {
        REQUIRE_THROWS_AS(parse(data), std::runtime_error);
      }
    }
  }

  GIVEN("A cell with a color past K") {
    auto data = buildCardData();
    data[57] = 11;

    WHEN("parse is called") {
      THEN("It throws a runtime_error") {
        REQUIRE_THROWS_AS(parse(data), std::runtime_error);
      }
    }
  }
}

SCENARIO("toJson writes the cards as rows of color letters") {
  GIVEN("Card 1 with diagonal stripes and card 2 all black") {
    CodeCards cards;
    for (size_t y = 0; y < consts::CARD_SIZE; y++) {
      for (size_t x = 0; x < consts::CARD_SIZE; x++) {
        cards[0].rows[y][x] = static_cast<uint8_t>((x + y) % 11);
      }
    }

    WHEN("toJson is called") {
      auto json = toJson(cards);

      THEN("It writes the color key and both cards") {
        const std::string expected = R"({
  "colors": {
    "A": "black",
    "B": "white",
    "C": "yellow",
    "D": "orange",
    "E": "red",
    "F": "brown",
    "G": "pink",
    "H": "light blue",
    "I": "dark blue",
    "J": "light green",
    "K": "dark green"
  },
  "cards": [
    {
      "card": 1,
      "rows": [
        "ABCDEFGHIJ",
        "BCDEFGHIJK",
        "CDEFGHIJKA",
        "DEFGHIJKAB",
        "EFGHIJKABC",
        "FGHIJKABCD",
        "GHIJKABCDE",
        "HIJKABCDEF",
        "IJKABCDEFG",
        "JKABCDEFGH"
      ]
    },
    {
      "card": 2,
      "rows": [
        "AAAAAAAAAA",
        "AAAAAAAAAA",
        "AAAAAAAAAA",
        "AAAAAAAAAA",
        "AAAAAAAAAA",
        "AAAAAAAAAA",
        "AAAAAAAAAA",
        "AAAAAAAAAA",
        "AAAAAAAAAA",
        "AAAAAAAAAA"
      ]
    }
  ]
}
)";
        REQUIRE(std::string(json.begin(), json.end()) == expected);
      }
    }
  }
}
