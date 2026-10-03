#include "../../../../src/engine/assets/Assets.h"

#include "../../../../src/engine/assets/DiskFiles.h"
#include "../../../TemporaryPath.h"

#include <catch2/catch_all.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace openfranko::src::engine;
using namespace openfranko::test;

namespace {

void touch(const std::filesystem::path &path) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream(path).put('\0');
}

} // namespace

SCENARIO("Resources are named after the data files of their version") {
  GIVEN("The 1.0 extraction") {
    THEN("Every resource is its hex file id") {
      REQUIRE(assets::resourceName(0x3BA, GameVersion::V10) == "03BA");
      REQUIRE(assets::resourceName(0x34, GameVersion::V10) == "0034");
      REQUIRE(assets::resourceName(0x261, GameVersion::V10) == "0261");
    }
  }

  GIVEN("The 1.2 extraction") {
    THEN("Sprite sets, tiles, tunes and screens take their 1.2 names") {
      REQUIRE(assets::resourceName(0x34, GameVersion::V12) == "s52");
      REQUIRE(assets::resourceName(0x137, GameVersion::V12) == "t11");
      REQUIRE(assets::resourceName(0x261, GameVersion::V12) == "m9");
      REQUIRE(assets::resourceName(0x384, GameVersion::V12) == "p0");
      REQUIRE(assets::resourceName(0x3B7, GameVersion::V12) == "p51");
      REQUIRE(assets::resourceName(0x3BA, GameVersion::V12) == "p54");
      REQUIRE(assets::resourceName(0x3C3, GameVersion::V12) == "p50");
    }

    THEN("Resources that 1.2 dropped are refused") {
      REQUIRE_THROWS_AS(assets::resourceName(0x260, GameVersion::V12),
                        std::runtime_error);
      REQUIRE_THROWS_AS(assets::resourceName(0x263, GameVersion::V12),
                        std::runtime_error);
    }
  }
}

SCENARIO("The version is told by the extracted files") {
  GIVEN("A directory holding a 1.0 extraction") {
    const TemporaryPath directory("openFrankoAssets10");
    touch(directory.path() / "0384/0384.bmp");

    THEN("It is version 1.0") {
      const assets::DiskFiles files;
      REQUIRE(assets::detectVersion(files, directory.path().string()) ==
              GameVersion::V10);
    }
  }

  GIVEN("A directory holding a 1.2 extraction") {
    const TemporaryPath directory("openFrankoAssets12");
    touch(directory.path() / "p0/p0.bmp");

    THEN("It is version 1.2") {
      const assets::DiskFiles files;
      REQUIRE(assets::detectVersion(files, directory.path().string()) ==
              GameVersion::V12);
    }
  }
}

SCENARIO("Asset paths follow the extractor's layout") {
  GIVEN("A directory with a bank's samples") {
    const TemporaryPath directory("openFrankoAssetsSamples");
    touch(directory.path() / "s50/s50_sam2_13160Hz.wav");
    touch(directory.path() / "s50/s50_sam20_8000Hz.wav");
    touch(directory.path() / "s50/s50_sam2_notes.txt");
    touch(directory.path() / "s50/s50_002.bmp");
    const std::string root = directory.path().string();

    THEN("Pictures, images, parts and tunes have fixed names") {
      REQUIRE(assets::picturePath("p54", root) == root + "/p54.bmp");
      REQUIRE(assets::imagePath("s50", 7, root) == root + "/s50/s50_007.bmp");
      REQUIRE(assets::imagePath("s50", 0, root) == root + "/s50/s50_000.bmp");
      REQUIRE(assets::imagePath("s50", 42, root) == root + "/s50/s50_042.bmp");
      REQUIRE(assets::imagePath("s50", 999, root) == root + "/s50/s50_999.bmp");
      REQUIRE(assets::imagePath("s50", 1000, root) ==
              root + "/s50/s50_1000.bmp");
      REQUIRE(assets::imagePath("s50", -1, root) == root + "/s50/s50_-01.bmp");
      REQUIRE(assets::partPath("p51", 0, root) == root + "/p51/p51.bmp");
      REQUIRE(assets::partPath("p51", 2, root) == root + "/p51/p51_2.bmp");
      REQUIRE(assets::musicPath("m11", root) == root + "/m11.s3m");
    }

    THEN("A sample is found whatever its rate") {
      const assets::DiskFiles files;
      REQUIRE(assets::samplePath(files, "s50", 2, root) ==
              root + "/s50/s50_sam2_13160Hz.wav");
    }

    THEN("A missing sample has no path") {
      const assets::DiskFiles files;
      REQUIRE(assets::samplePath(files, "s50", 3, root).empty());
      REQUIRE(assets::samplePath(files, "s51", 1, root).empty());
    }
  }
}
