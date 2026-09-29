#include "../../../lib/json/json.h"

#include <catch2/catch_all.hpp>

#include <string>

using namespace openfranko::lib::json;

SCENARIO("escape makes text safe inside a JSON string") {
  GIVEN("Plain text") {
    THEN("It stays as it is") {
      REQUIRE(escape("Franko: The Crazy Revenge") ==
              "Franko: The Crazy Revenge");
    }
  }

  GIVEN("Quotes and backslashes") {
    THEN("Each gets a backslash in front") {
      REQUIRE(escape("\"a\\b\"") == "\\\"a\\\\b\\\"");
    }
  }

  GIVEN("Control characters") {
    THEN("They become \\u escapes") {
      REQUIRE(escape(std::string("a\nb\x01", 4)) == "a\\u000ab\\u0001");
    }
  }
}
