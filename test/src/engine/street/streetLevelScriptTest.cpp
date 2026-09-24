#include "../../../../src/engine/street/LevelScript.h"
#include <catch2/catch_all.hpp>
#include <stdexcept>

using namespace openfranko::src::engine::street;

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
