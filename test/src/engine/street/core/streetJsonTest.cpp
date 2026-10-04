#include "../../../../../src/engine/street/core/Json.h"

#include <catch2/catch_all.hpp>

#include <stdexcept>
#include <string>

using namespace openfranko::src::engine::street::core;

SCENARIO("JSON documents are read into values") {
  GIVEN("Numbers written in every form") {
    const JsonValue numbers = parseJson(
        "[0, -104, 568, 123456789, 1234567890, -2147483648, 2.5, -1e3, 75E-1]");
    const auto &items = numbers.array();

    THEN("Whole numbers are integers and the rest keep their fractions") {
      REQUIRE(items.size() == 9);
      REQUIRE(items[0].integer() == 0);
      REQUIRE(items[1].integer() == -104);
      REQUIRE(items[2].integer() == 568);
      REQUIRE(items[3].integer() == 123456789);
      REQUIRE(items[4].integer() == 1234567890);
      REQUIRE(items[5].number == -2147483648.0);
      REQUIRE(items[6].number == 2.5);
      REQUIRE(items[7].integer() == -1000);
      REQUIRE(items[8].number == 7.5);
      REQUIRE_THROWS_AS(items[6].integer(), std::invalid_argument);
    }
  }

  GIVEN("Keywords, escaped strings and nested containers") {
    const JsonValue root = parseJson(
        R"({ "yes": true, "no": false, "none": null, "text": "a\"b\\c",
             "nested": [[], {}] })");

    THEN("Each member keeps its kind and value") {
      REQUIRE(root.member("yes").kind == JsonValue::Kind::Boolean);
      REQUIRE(root.member("yes").boolean);
      REQUIRE_FALSE(root.member("no").boolean);
      REQUIRE(root.member("none").kind == JsonValue::Kind::Null);
      REQUIRE(root.member("text").string() == "a\"b\\c");
      REQUIRE(root.member("nested").array().size() == 2);
      REQUIRE(root.member("nested").array()[1].kind == JsonValue::Kind::Object);
    }

    THEN("A missing member is named") {
      REQUIRE_THROWS_WITH(root.member("gone"), "JSON: missing \"gone\"");
    }
  }

  GIVEN("Broken documents") {
    THEN("They are refused at the offset of the fault") {
      REQUIRE_THROWS_WITH(parseJson("[1, 2"),
                          "JSON: unexpected end at offset 5");
      REQUIRE_THROWS_WITH(parseJson("[1] x"),
                          "JSON: trailing characters at offset 4");
      REQUIRE_THROWS_WITH(parseJson("[-]"),
                          "JSON: unexpected character at offset 1");
      REQUIRE_THROWS_WITH(parseJson(R"({"a" 1})"),
                          "JSON: expected ':' at offset 5");
      REQUIRE_THROWS_AS(parseJson("[nul]"), std::invalid_argument);
    }
  }
}

SCENARIO("A JSON cursor reads values one at a time") {
  GIVEN("An object with numbers, text and nested values") {
    JsonCursor cursor(
        R"( {"a": -1e3, "b": "t\"x", "c": [1, {"d": null}], "e": 4, "f": 7} )");

    THEN("Members are visited in order and skipped values leave no trace") {
      std::string key;
      cursor.openObject();
      REQUIRE(cursor.nextMember(key));
      REQUIRE(key == "a");
      REQUIRE(cursor.integer() == -1000);
      REQUIRE(cursor.nextMember(key));
      REQUIRE(key == "b");
      REQUIRE(cursor.text() == "t\"x");
      REQUIRE(cursor.nextMember(key));
      REQUIRE(key == "c");
      cursor.skip();
      REQUIRE(cursor.nextMember(key));
      REQUIRE(key == "e");
      REQUIRE_FALSE(cursor.skipNull());
      REQUIRE(cursor.integer() == 4);
      REQUIRE(cursor.nextMember(key));
      REQUIRE(cursor.value().integer() == 7);
      REQUIRE_FALSE(cursor.nextMember(key));
      cursor.finish();
    }
  }

  GIVEN("Values of the wrong kind") {
    THEN("They are refused like the values of a document") {
      REQUIRE_THROWS_WITH(JsonCursor("2.5").integer(),
                          "JSON: expected an integer");
      REQUIRE_THROWS_WITH(JsonCursor("\"9\"").integer(),
                          "JSON: expected an integer");
      REQUIRE_THROWS_WITH(JsonCursor("[]").text(), "JSON: expected a string");
      REQUIRE_THROWS_WITH(JsonCursor("[1 2]").skip(),
                          "JSON: expected ']' at offset 3");
    }
  }
}
