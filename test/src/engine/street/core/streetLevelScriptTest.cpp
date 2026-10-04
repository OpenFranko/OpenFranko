#include "../../../../../src/engine/street/core/LevelScript.h"

#include <catch2/catch_all.hpp>

#include <stdexcept>

using namespace openfranko::src::engine::street::core;

namespace {

const char *const SCRIPT = R"({
  "fileId": "0385",
  "lengthInColumns": 568,
  "waveCount": 2,
  "waves": [
    {
      "triggerColumn": 12,
      "slots": [
        { "spriteSetId": 1, "kind": 0, "kindName": "bald", "spawnX": 336, "spawnY": 200, "unused": 0, "energy": 5, "aggression": 25 },
        null,
        null
      ]
    },
    {
      "triggerColumn": 25,
      "slots": [
        { "spriteSetId": 1, "kind": 0, "kindName": "bald", "spawnX": -104, "spawnY": 196, "unused": 0, "energy": 10, "aggression": 20 },
        { "spriteSetId": 7, "kind": 1, "kindName": "frog", "spawnX": 420, "spawnY": 184, "unused": 0, "energy": 20, "aggression": 20 },
        null
      ]
    }
  ]
})";

} // namespace

SCENARIO("A level script is read from the extractor's JSON") {
  GIVEN("The first two waves of stage 1") {
    const LevelScript script = LevelScript::fromJson(SCRIPT);

    THEN("The level length and every slot field are kept") {
      REQUIRE(script.length == 568);
      REQUIRE(script.waves.size() == 2);
      REQUIRE(script.waves[0].trigger == 12);
      const EnemySlot &bald = script.waves[0].slots[0];
      REQUIRE(bald.spriteSet == 1);
      REQUIRE(bald.type == 0);
      REQUIRE(bald.x == 336);
      REQUIRE(bald.y == 200);
      REQUIRE(bald.energy == 5);
      REQUIRE(bald.aggression == 25);
    }

    THEN("A null slot is empty, and a spawn X left of the screen is negative") {
      REQUIRE(script.waves[0].slots[1].spriteSet == EnemySlot::EMPTY);
      REQUIRE(script.waves[1].slots[0].x == -104);
      REQUIRE(script.waves[1].slots[1].type == 1);
    }
  }

  GIVEN("Broken scripts") {
    THEN("They are refused") {
      REQUIRE_THROWS_AS(LevelScript::fromJson("{}"), std::invalid_argument);
      REQUIRE_THROWS_AS(
          LevelScript::fromJson(R"({"lengthInColumns": 5, "waves": [)"),
          std::invalid_argument);
      REQUIRE_THROWS_AS(
          LevelScript::fromJson(
              R"({"lengthInColumns": 5, "waves": [{"triggerColumn": 1, "slots": [null]}]})"),
          std::invalid_argument);
    }
  }
}

SCENARIO("A level script is read one wave per step") {
  GIVEN("The first two waves of stage 1") {
    LevelScriptReader reader(SCRIPT);
    LevelScript script;

    THEN("The header takes a step and each wave one more") {
      REQUIRE_FALSE(reader.step(script));
      REQUIRE(script.length == 568);
      REQUIRE(script.waves.empty());
      REQUIRE_FALSE(reader.step(script));
      REQUIRE(script.waves.size() == 1);
      REQUIRE_FALSE(reader.step(script));
      REQUIRE(script.waves.size() == 2);
      REQUIRE(reader.step(script));
      REQUIRE(script.waves[1].slots[1].x == 420);
    }
  }

  GIVEN("Members in another order, repeated and unknown") {
    const LevelScript script = LevelScript::fromJson(R"({
      "waves": [{
        "slots": [null, {"aggression": 3, "energy": 4, "spawnY": 5,
                         "spawnX": -6, "kind": 7, "kind": 70,
                         "spriteSetId": 8, "extra": {"a": [1, "x\"y"]}},
                  null],
        "triggerColumn": 9}],
      "lengthInColumns": 10,
      "lengthInColumns": 11,
      "note": [true, false, null, 2.5]})");

    THEN("Each field is found and the first copy of a member wins") {
      REQUIRE(script.length == 10);
      REQUIRE(script.waves.size() == 1);
      REQUIRE(script.waves[0].trigger == 9);
      const EnemySlot &slot = script.waves[0].slots[1];
      REQUIRE(slot.spriteSet == 8);
      REQUIRE(slot.type == 7);
      REQUIRE(slot.x == -6);
      REQUIRE(slot.y == 5);
      REQUIRE(slot.energy == 4);
      REQUIRE(slot.aggression == 3);
      REQUIRE(script.waves[0].slots[0].spriteSet == EnemySlot::EMPTY);
    }
  }

  GIVEN("Scripts missing a member") {
    THEN("The member is named") {
      REQUIRE_THROWS_WITH(
          LevelScript::fromJson(R"({"lengthInColumns": 1, "waves": [
            {"triggerColumn": 1, "slots": [{"spriteSetId": 1}, null, null]}]})"),
          "JSON: missing \"kind\"");
      REQUIRE_THROWS_WITH(LevelScript::fromJson(R"({"waves": []})"),
                          "JSON: missing \"lengthInColumns\"");
      REQUIRE_THROWS_WITH(
          LevelScript::fromJson(R"({"lengthInColumns": 1, "waves": [{}]})"),
          "JSON: missing \"triggerColumn\"");
    }
  }
}
