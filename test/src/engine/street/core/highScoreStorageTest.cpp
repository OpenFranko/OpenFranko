#include "../../../../../src/engine/street/core/HighScoreStorage.h"

#include <catch2/catch_all.hpp>

#include <optional>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street::core;

SCENARIO("High scores fit the 63 data words of the cartridge EEPROM") {
  GIVEN("The tables a fresh game starts with") {
    for (const GameVersion version : {GameVersion::V10, GameVersion::V12}) {
      const HighScoreTable table(version);
      const std::optional<HighScoreTable> unpacked =
          unpackHighScores(packHighScores(table));

      THEN("They come back byte for byte") {
        REQUIRE(unpacked.has_value());
        REQUIRE(unpacked->bytes() == table.bytes());
      }
    }
  }

  GIVEN("A table with new scores and typed names") {
    HighScoreTable table(GameVersion::V10);
    table.setName(table.insert(250), "ZBIGNIEW BONIEK");
    table.setName(table.insert(7), "A B");
    table.setName(table.insert(1), "");
    const std::optional<HighScoreTable> unpacked =
        unpackHighScores(packHighScores(table));

    THEN("Names, spaces and scores survive the packing") {
      REQUIRE(unpacked.has_value());
      REQUIRE(unpacked->bytes() == table.bytes());
      REQUIRE(unpacked->score(0) == 250);
    }
  }

  GIVEN("Words that were never written") {
    StorageWords blank{};
    blank.fill(0xFFFF);

    THEN("They are not taken for a table") {
      REQUIRE_FALSE(unpackHighScores(blank).has_value());
    }
  }
}
