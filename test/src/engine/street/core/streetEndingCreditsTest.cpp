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
      REQUIRE_THROWS_WITH(
          EndingCredits::fromJson(
              R"({"pages": [{"beat": 1, "lines": [{"text": "A"}]}]})"),
          "JSON: missing \"y\"");
      REQUIRE_THROWS_WITH(
          EndingCredits::fromJson(R"({"pages": [{"lines": []}]})"),
          "JSON: missing \"beat\"");
      REQUIRE_THROWS_WITH(
          EndingCredits::fromJson(R"({"pages": [{"beat": 1}]})"),
          "JSON: missing \"lines\"");
      REQUIRE_THROWS_AS(EndingCredits::fromJson(R"({"pages": [)"),
                        std::invalid_argument);
    }
  }
}

SCENARIO("Credit pages keep the first copy of their members") {
  GIVEN("A page and a line with repeated and unknown members") {
    const EndingCredits credits = EndingCredits::fromJson(R"({"pages": [
      {"beat": 5, "beat": 9, "colour": 3,
       "lines": [{"text": "A", "text": "B", "y": 2, "y": 4, "x": 1}],
       "lines": []}]})");

    THEN("The first beat, lines, text and y are kept") {
      REQUIRE(credits.pages.size() == 1);
      REQUIRE(credits.pages[0].beat == 5);
      REQUIRE(credits.pages[0].lines.size() == 1);
      REQUIRE(credits.pages[0].lines[0].text == "A");
      REQUIRE(credits.pages[0].lines[0].y == 2);
    }
  }
}
