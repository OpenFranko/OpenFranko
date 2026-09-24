#include "../../../../src/engine/street/HighScoreTable.h"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace openfranko::src::engine::street;

namespace {

std::string nameOf(const HighScoreTable &table, int row) {
  std::string name;
  for (int column = 0; column < HighScoreTable::NAME_LENGTH; ++column) {
    const int letter = table.letter(row, column);
    name += letter < HighScoreTable::LETTERS ? static_cast<char>('A' + letter)
                                             : '.';
  }
  return name;
}

HighScoreTable ladder(int lowest) {
  HighScoreTable table;
  for (int step = 0; step < HighScoreTable::ROWS; ++step) {
    const int slot = table.insert(lowest + step * 10);
    table.setName(slot, std::string(1, static_cast<char>('A' + step)));
  }
  return table;
}

std::filesystem::path scratchFile() {
  return std::filesystem::temp_directory_path() / "openFrankoHighScoreTest_h";
}

} // namespace

SCENARIO("HINEW seeds ten WORLD SOFTWARE entries worth nothing") {
  GIVEN("A table with no file behind it") {
    const HighScoreTable table;

    THEN("Every row spells WORLD SOFTWARE with 0 kills") {
      for (int row = 0; row < HighScoreTable::ROWS; ++row) {
        REQUIRE(nameOf(table, row) == "WORLD.SOFTWARE.");
        REQUIRE(table.score(row) == 0);
      }
    }

    THEN("The gaps are the seed's space shifted by -66, drawn as nothing") {
      REQUIRE(table.letter(0, 5) == 0xDE);
      REQUIRE(table.letter(9, 14) == 0xDE);
    }
  }
}

SCENARIO("The file stores every long of the table rotated right by 6 bits") {
  GIVEN("A file whose first long is $04000000 and second $FC000000") {
    HighScoreTable::Bytes file{};
    file[0] = 0x04;
    file[4] = 0xFC;
    const HighScoreTable table = HighScoreTable::fromFile(file);

    THEN("Rol.l 6 on load turns them into $00000001 and $0000003F") {
      REQUIRE(table.letter(0, 0) == 0);
      REQUIRE(table.letter(0, 3) == 1);
      REQUIRE(table.letter(0, 4) == 0);
      REQUIRE(table.letter(0, 7) == 0x3F);
    }

    THEN("Ror.l 6 before the Bsave writes the same bytes back") {
      REQUIRE(table.toFile() == file);
    }
  }
}

SCENARIO("A score takes the first slot it equals or beats") {
  GIVEN("Scores 90 down to 0 named J down to A") {
    HighScoreTable table = ladder(0);

    WHEN("80 kills are entered") {
      const int slot = table.insert(80);

      THEN("It goes above the old 80, which moves down with the rest") {
        REQUIRE(slot == 1);
        REQUIRE(table.score(1) == 80);
        REQUIRE(nameOf(table, 2).front() == 'I');
        REQUIRE(table.score(2) == 80);
        REQUIRE(nameOf(table, 9).front() == 'B');
      }

      THEN("The new name is blanked to $FF and the score byte kept") {
        for (int column = 0; column < HighScoreTable::NAME_LENGTH; ++column) {
          REQUIRE(table.letter(1, column) == 0xFF);
        }
      }
    }

    WHEN("5 kills are entered") {
      const int slot = table.insert(5);

      THEN("The last row is overwritten without any shifting") {
        REQUIRE(slot == 9);
        REQUIRE(table.score(9) == 5);
        REQUIRE(nameOf(table, 8).front() == 'B');
      }
    }

    WHEN("300 kills are entered") {
      const int slot = table.insert(300);

      THEN("It tops the table but Poke keeps only the low byte") {
        REQUIRE(slot == 0);
        REQUIRE(table.score(0) == 300 - 256);
      }
    }
  }

  GIVEN("Scores 100 down to 10") {
    HighScoreTable table = ladder(10);
    const HighScoreTable::Bytes before = table.bytes();

    WHEN("A run with 5 kills ends") {
      const int slot = table.insert(5);

      THEN("No slot is found and nothing is written") {
        REQUIRE(slot == HighScoreTable::NO_SLOT);
        REQUIRE(table.bytes() == before);
      }
    }
  }
}

SCENARIO("Return pokes each typed character as Asc()-65") {
  GIVEN("A fresh slot") {
    HighScoreTable table;
    const int slot = table.insert(12);

    WHEN("The name AB C is committed") {
      table.setName(slot, "AB C           ");

      THEN("Letters are 0-25 and spaces $DF, all fifteen bytes written") {
        REQUIRE(table.letter(slot, 0) == 0);
        REQUIRE(table.letter(slot, 1) == 1);
        REQUIRE(table.letter(slot, 2) == 0xDF);
        REQUIRE(table.letter(slot, 3) == 2);
        REQUIRE(table.letter(slot, 14) == 0xDF);
        REQUIRE(table.score(slot) == 12);
      }
    }
  }
}

SCENARIO("The table is saved and loaded as the rotated 160-byte file") {
  GIVEN("A table with an entry") {
    HighScoreTable table;
    table.setName(table.insert(42), "NICE           ");
    const std::filesystem::path path = scratchFile();

    WHEN("It is written and read back") {
      REQUIRE(writeHighScoreFile(table, path.string()));
      std::ifstream file(path, std::ios::binary);
      const std::vector<char> written((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
      const auto loaded = readHighScoreFile(path.string());
      std::filesystem::remove(path);

      THEN("The file holds exactly the rotated table") {
        const HighScoreTable::Bytes rotated = table.toFile();
        REQUIRE(written.size() ==
                static_cast<std::size_t>(HighScoreTable::SIZE));
        REQUIRE(std::equal(
            written.begin(), written.end(), rotated.begin(),
            [](char a, uint8_t b) { return static_cast<uint8_t>(a) == b; }));
      }

      THEN("Loading it gives the same table") {
        REQUIRE(loaded.has_value());
        REQUIRE(loaded->bytes() == table.bytes());
      }
    }
  }

  GIVEN("No file") {
    std::filesystem::remove(scratchFile());

    THEN("There is nothing to load, so boot falls back to HINEW") {
      REQUIRE_FALSE(readHighScoreFile(scratchFile().string()).has_value());
    }
  }
}
