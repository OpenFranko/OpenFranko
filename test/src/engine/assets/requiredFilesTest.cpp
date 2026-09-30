#include "../../../../src/engine/assets/RequiredFiles.h"

#include "FakeFiles.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::assets;
using namespace openfranko::test::src::engine::assets;

namespace {

bool contains(const std::vector<std::string> &paths, const std::string &path) {
  return std::find(paths.begin(), paths.end(), path) != paths.end();
}

FakeFiles filesWith(const std::vector<std::string> &paths) {
  FakeFiles files;
  for (const std::string &path : paths) {
    files.contents[path] = {};
  }
  return files;
}

} // namespace

SCENARIO("requiredFiles names a file of every resource the game loads") {
  GIVEN("Franko 1.0") {
    const std::vector<std::string> paths = requiredFiles(GameVersion::V10);

    THEN("Each kind of resource is named by its 1.0 file") {
      REQUIRE(contains(paths, "assets/0000/0000_000.bmp"));
      REQUIRE(contains(paths, "assets/0154/0154_000.bmp"));
      REQUIRE(contains(paths, "assets/0384/0384.bmp"));
      REQUIRE(contains(paths, "assets/0385.json"));
      REQUIRE(contains(paths, "assets/0262.s3m"));
      REQUIRE(contains(paths, "assets/03B6.bmp"));
      REQUIRE(contains(paths, "assets/03C3.bmp"));
      REQUIRE(contains(paths, "assets/0263/0263_sam1_13160Hz.wav"));
      REQUIRE(contains(paths, "assets/0384/0384_cards.bin"));
      REQUIRE(contains(paths, "assets/credits.json"));
    }

    THEN("Files only 1.2 has are not required") {
      REQUIRE_FALSE(contains(paths, "assets/0032/0032_000.bmp"));
      REQUIRE_FALSE(contains(paths, "assets/m11.s3m"));
    }
  }

  GIVEN("Franko 1.2") {
    const std::vector<std::string> paths = requiredFiles(GameVersion::V12);

    THEN("Each kind of resource is named by its 1.2 file") {
      REQUIRE(contains(paths, "assets/s0/s0_000.bmp"));
      REQUIRE(contains(paths, "assets/s50/s50_000.bmp"));
      REQUIRE(contains(paths, "assets/t40/t40_000.bmp"));
      REQUIRE(contains(paths, "assets/p0/p0.bmp"));
      REQUIRE(contains(paths, "assets/p1.json"));
      REQUIRE(contains(paths, "assets/m11.s3m"));
      REQUIRE(contains(paths, "assets/p50.bmp"));
      REQUIRE(contains(paths, "assets/p85.bmp"));
      REQUIRE(contains(paths, "assets/credits.json"));
    }

    THEN("The optional intro texts are not required") {
      REQUIRE_FALSE(contains(paths, "assets/intro.json"));
    }
  }
}

SCENARIO("missingFiles lists the required files that are not there") {
  GIVEN("Every required 1.0 file") {
    FakeFiles files = filesWith(requiredFiles(GameVersion::V10));

    THEN("Nothing is missing") {
      REQUIRE(missingFiles(files, GameVersion::V10).empty());
    }

    WHEN("A picture and the ending credits are removed") {
      files.contents.erase("assets/03B6.bmp");
      files.contents.erase("assets/credits.json");

      THEN("Exactly those two are missing") {
        REQUIRE(
            missingFiles(files, GameVersion::V10) ==
            std::vector<std::string>{"assets/03B6.bmp", "assets/credits.json"});
      }
    }

    WHEN("The same files are checked as Franko 1.2") {
      THEN("The 1.2 files are missing") {
        REQUIRE(contains(missingFiles(files, GameVersion::V12),
                         "assets/s0/s0_000.bmp"));
      }
    }
  }
}
