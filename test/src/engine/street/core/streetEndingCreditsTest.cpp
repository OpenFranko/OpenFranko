#include "../../../../../src/engine/street/core/EndingCredits.h"

#include <catch2/catch_all.hpp>

#include <stdexcept>
#include <string>

using namespace openfranko::src::engine::street::core;

namespace {

const char *const CREDITS = R"({
  "note": {"skipped": [1, "two"]},
  "pages": [
    {"beat": 40, "lines": [{"y": 10, "text": "FRANKO"},
                           {"text": "THE CRAZY REVENGE", "y": 30}]},
    {"lines": [], "beat": 0},
    {"beat": 7, "lines": [{"y": 5, "text": "THE \"END\""}]}
  ]
})";

} // namespace

SCENARIO("Ending credits are read from the extractor's JSON") {
  GIVEN("Three pages, one of them empty") {
    const EndingCredits credits = EndingCredits::fromJson(CREDITS);

    THEN("Each page keeps its beat and its lines in order") {
      REQUIRE(credits.pages.size() == 3);
      REQUIRE(credits.pages[0].beat == 40);
      REQUIRE(credits.pages[0].lines.size() == 2);
      REQUIRE(credits.pages[0].lines[1].text == "THE CRAZY REVENGE");
      REQUIRE(credits.pages[0].lines[1].y == 30);
      REQUIRE(credits.pages[1].lines.empty());
      REQUIRE(credits.pages[2].lines[0].text == "THE \"END\"");
    }
  }

  GIVEN("The same pages read in steps") {
    EndingCreditsReader reader(CREDITS);
    EndingCredits credits;

    THEN("The header takes a step and each page one more") {
      REQUIRE_FALSE(reader.step(credits));
      REQUIRE(credits.pages.empty());
      REQUIRE_FALSE(reader.step(credits));
      REQUIRE(credits.pages.size() == 1);
      REQUIRE_FALSE(reader.step(credits));
      REQUIRE_FALSE(reader.step(credits));
      REQUIRE(credits.pages.size() == 3);
      REQUIRE(reader.step(credits));
    }
  }

  GIVEN("Credits missing a member") {
    THEN("The member is named") {
      REQUIRE_THROWS_WITH(EndingCredits::fromJson(R"({"pagez": []})"),
                          "JSON: missing \"pages\"");
      REQUIRE_THROWS_WITH(
          EndingCredits::fromJson(R"({"pages": [{"beat": 1, "lines": [{}]}]})"),
          "JSON: missing \"text\"");
      REQUIRE_THROWS_AS(EndingCredits::fromJson(R"({"pages": [)"),
                        std::invalid_argument);
    }
  }
}
