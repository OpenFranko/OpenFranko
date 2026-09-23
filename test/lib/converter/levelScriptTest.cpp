#include "../../../lib/converter/levelScript/levelScript.h"
#include "../../../lib/helpers/helpers.h"
#include <catch2/catch_all.hpp>
#include <string>
#include <string_view>
#include <vector>

using namespace openfranko::lib::converter::levelScript;
using namespace openfranko::lib::helpers;

namespace {

void pushEnemy(std::vector<uint8_t> &data, uint8_t spriteSetId, uint8_t kind,
               int16_t spawnX, uint8_t spawnY, uint8_t unused, uint8_t energy,
               uint8_t aggression) {
  data.push_back(spriteSetId);
  data.push_back(kind);
  pushBigEndian16(data, static_cast<uint16_t>(spawnX));
  data.push_back(spawnY);
  data.push_back(unused);
  data.push_back(energy);
  data.push_back(aggression);
}

void pushEmptySlot(std::vector<uint8_t> &data) {
  data.insert(data.end(), {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
}

EnemySlot enemy(uint8_t spriteSetId, uint8_t kind, int16_t spawnX,
                uint8_t spawnY, uint8_t unused, uint8_t energy,
                uint8_t aggression) {
  EnemySlot slot;
  slot.occupied = true;
  slot.spriteSetId = spriteSetId;
  slot.kind = kind;
  slot.spawnX = spawnX;
  slot.spawnY = spawnY;
  slot.unused = unused;
  slot.energy = energy;
  slot.aggression = aggression;
  return slot;
}

} // namespace

SCENARIO("parse reads the level length and enemy waves") {
  GIVEN("A level script with two waves") {
    std::vector<uint8_t> data;
    pushBigEndian16(data, 568);

    pushBigEndian16(data, 300);
    pushEnemy(data, 14, 2, -380, 200, 3, 20, 10);
    pushEmptySlot(data);
    pushEnemy(data, 7, 1, 420, 184, 0, 40, 30);

    pushBigEndian16(data, 551);
    pushEnemy(data, 1, 0, 336, 172, 0, 55, 0);
    pushEmptySlot(data);
    pushEmptySlot(data);

    WHEN("parse is called") {
      auto level = parse(data);

      THEN("The level length is read as big-endian") {
        REQUIRE(level.lengthInColumns == 568);
      }

      THEN("Both waves are read in order") {
        REQUIRE(level.waves.size() == 2);
        REQUIRE(level.waves[0].triggerColumn == 300);
        REQUIRE(level.waves[1].triggerColumn == 551);
      }

      THEN("An occupied slot has all its fields, with a signed spawnX") {
        const auto &slot = level.waves[0].slots[0];
        REQUIRE(slot.occupied);
        REQUIRE(slot.spriteSetId == 14);
        REQUIRE(slot.kind == 2);
        REQUIRE(slot.spawnX == -380);
        REQUIRE(slot.spawnY == 200);
        REQUIRE(slot.unused == 3);
        REQUIRE(slot.energy == 20);
        REQUIRE(slot.aggression == 10);
      }

      THEN("A sprite set id of 0xFF marks an empty slot") {
        REQUIRE_FALSE(level.waves[0].slots[1].occupied);
        REQUIRE(level.waves[0].slots[1].spriteSetId == consts::EMPTY_SLOT);
        REQUIRE_FALSE(level.waves[1].slots[1].occupied);
        REQUIRE_FALSE(level.waves[1].slots[2].occupied);
      }

      THEN("A slot after an empty slot is read from its own offset") {
        const auto &slot = level.waves[0].slots[2];
        REQUIRE(slot.occupied);
        REQUIRE(slot.spriteSetId == 7);
        REQUIRE(slot.kind == 1);
        REQUIRE(slot.spawnX == 420);
        REQUIRE(slot.spawnY == 184);
        REQUIRE(slot.unused == 0);
        REQUIRE(slot.energy == 40);
        REQUIRE(slot.aggression == 30);
      }

      THEN("The second wave is read from its own offset") {
        const auto &slot = level.waves[1].slots[0];
        REQUIRE(slot.occupied);
        REQUIRE(slot.spriteSetId == 1);
        REQUIRE(slot.kind == 0);
        REQUIRE(slot.spawnX == 336);
        REQUIRE(slot.spawnY == 172);
        REQUIRE(slot.unused == 0);
        REQUIRE(slot.energy == 55);
        REQUIRE(slot.aggression == 0);
      }
    }
  }

  GIVEN("A level script with a header only") {
    std::vector<uint8_t> data;
    pushBigEndian16(data, 316);

    WHEN("parse is called") {
      auto level = parse(data);

      THEN("It has the level length and no waves") {
        REQUIRE(level.lengthInColumns == 316);
        REQUIRE(level.waves.empty());
      }
    }
  }
}

SCENARIO("parse rejects malformed level scripts") {
  GIVEN("An empty buffer") {
    std::vector<uint8_t> data;

    WHEN("parse is called") {
      THEN("It throws a runtime_error") {
        REQUIRE_THROWS_AS(parse(data), std::runtime_error);
      }
    }
  }

  GIVEN("A buffer smaller than the header") {
    std::vector<uint8_t> data = {0x02};

    WHEN("parse is called") {
      THEN("It throws a runtime_error") {
        REQUIRE_THROWS_AS(parse(data), std::runtime_error);
      }
    }
  }

  GIVEN("A wave missing its last slot") {
    std::vector<uint8_t> data;
    pushBigEndian16(data, 316);
    pushBigEndian16(data, 12);
    pushEnemy(data, 1, 0, 336, 200, 0, 5, 25);
    pushEmptySlot(data);

    WHEN("parse is called") {
      THEN("It throws a runtime_error") {
        REQUIRE_THROWS_AS(parse(data), std::runtime_error);
      }
    }
  }

  GIVEN("A complete wave followed by a stray byte") {
    std::vector<uint8_t> data;
    pushBigEndian16(data, 316);
    pushBigEndian16(data, 12);
    pushEnemy(data, 1, 0, 336, 200, 0, 5, 25);
    pushEmptySlot(data);
    pushEmptySlot(data);
    data.push_back(0x00);

    WHEN("parse is called") {
      THEN("It throws a runtime_error") {
        REQUIRE_THROWS_AS(parse(data), std::runtime_error);
      }
    }
  }
}

SCENARIO("enemyKinds::name maps enemy kinds to names") {
  GIVEN("The enemy kinds used by the game") {
    THEN("0, 1 and 2 are bald, frog and kid") {
      REQUIRE(std::string_view(enemyKinds::name(0)) == "bald");
      REQUIRE(std::string_view(enemyKinds::name(1)) == "frog");
      REQUIRE(std::string_view(enemyKinds::name(2)) == "kid");
    }
  }

  GIVEN("Kinds outside the known range") {
    THEN("They are unknown") {
      REQUIRE(std::string_view(enemyKinds::name(3)) == "unknown");
      REQUIRE(std::string_view(enemyKinds::name(0xFF)) == "unknown");
    }
  }
}

SCENARIO("toJson writes a level as JSON") {
  GIVEN("A level with two waves") {
    Level level;
    level.lengthInColumns = 568;

    Wave first;
    first.triggerColumn = 25;
    first.slots[0] = enemy(1, 0, -104, 196, 3, 10, 20);
    first.slots[2] = enemy(7, 1, 420, 184, 0, 40, 30);
    level.waves.push_back(first);

    Wave second;
    second.triggerColumn = 42;
    second.slots[0] = enemy(14, 2, 420, 172, 0, 20, 10);
    level.waves.push_back(second);

    WHEN("toJson is called") {
      auto json = toJson(level, "0385");

      THEN("It writes every wave and slot, with null for empty slots") {
        const std::string expected = R"({
  "fileId": "0385",
  "lengthInColumns": 568,
  "waveCount": 2,
  "waves": [
    {
      "triggerColumn": 25,
      "slots": [
        { "spriteSetId": 1, "kind": 0, "kindName": "bald", "spawnX": -104, "spawnY": 196, "unused": 3, "energy": 10, "aggression": 20 },
        null,
        { "spriteSetId": 7, "kind": 1, "kindName": "frog", "spawnX": 420, "spawnY": 184, "unused": 0, "energy": 40, "aggression": 30 }
      ]
    },
    {
      "triggerColumn": 42,
      "slots": [
        { "spriteSetId": 14, "kind": 2, "kindName": "kid", "spawnX": 420, "spawnY": 172, "unused": 0, "energy": 20, "aggression": 10 },
        null,
        null
      ]
    }
  ]
}
)";
        REQUIRE(std::string(json.begin(), json.end()) == expected);
      }
    }
  }

  GIVEN("A level without waves") {
    Level level;
    level.lengthInColumns = 316;

    WHEN("toJson is called") {
      auto json = toJson(level, "0386");

      THEN("It writes an empty waves array") {
        const std::string expected = R"({
  "fileId": "0386",
  "lengthInColumns": 316,
  "waveCount": 0,
  "waves": [
  ]
}
)";
        REQUIRE(std::string(json.begin(), json.end()) == expected);
      }
    }
  }

  GIVEN("A file id with characters that JSON must escape") {
    Level level;
    level.lengthInColumns = 316;

    WHEN("toJson is called") {
      auto json = toJson(level, "a\"b\\c\nd\x01");

      THEN("Quotes, backslashes and control characters are escaped") {
        const std::string expected = R"({
  "fileId": "a\"b\\c\u000ad\u0001",
  "lengthInColumns": 316,
  "waveCount": 0,
  "waves": [
  ]
}
)";
        REQUIRE(std::string(json.begin(), json.end()) == expected);
      }
    }
  }
}
