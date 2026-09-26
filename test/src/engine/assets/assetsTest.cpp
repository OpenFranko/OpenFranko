#include "../../../../src/engine/assets/Assets.h"
#include <catch2/catch_all.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace openfranko::src::engine;

namespace {

class TemporaryDirectory {
public:
  explicit TemporaryDirectory(const std::string &name)
      : m_path(std::filesystem::temp_directory_path() / name) {
    std::filesystem::remove_all(m_path);
    std::filesystem::create_directories(m_path);
  }
  ~TemporaryDirectory() { std::filesystem::remove_all(m_path); }

  void touch(const std::string &file) const {
    const std::filesystem::path path = m_path / file;
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path).put('\0');
  }

  std::string path() const { return m_path.string(); }

private:
  std::filesystem::path m_path;
};

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
    const TemporaryDirectory directory("openfranko-assets-10");
    directory.touch("0384/0384.bmp");

    THEN("It is version 1.0") {
      REQUIRE(assets::detectVersion(directory.path()) == GameVersion::V10);
    }
  }

  GIVEN("A directory holding a 1.2 extraction") {
    const TemporaryDirectory directory("openfranko-assets-12");
    directory.touch("p0/p0.bmp");

    THEN("It is version 1.2") {
      REQUIRE(assets::detectVersion(directory.path()) == GameVersion::V12);
    }
  }
}

SCENARIO("Asset paths follow the extractor's layout") {
  GIVEN("A directory with a bank's samples") {
    const TemporaryDirectory directory("openfranko-assets-samples");
    directory.touch("s50/s50_sam2_13160Hz.wav");
    directory.touch("s50/s50_002.bmp");
    const std::string root = directory.path();

    THEN("Pictures, images, parts and tunes have fixed names") {
      REQUIRE(assets::picturePath("p54", root) == root + "/p54.bmp");
      REQUIRE(assets::imagePath("s50", 7, root) == root + "/s50/s50_007.bmp");
      REQUIRE(assets::partPath("p51", 0, root) == root + "/p51/p51.bmp");
      REQUIRE(assets::partPath("p51", 2, root) == root + "/p51/p51_2.bmp");
      REQUIRE(assets::musicPath("m11", root) == root + "/m11.s3m");
    }

    THEN("A sample is found whatever its rate") {
      REQUIRE(assets::samplePath("s50", 2, root) ==
              root + "/s50/s50_sam2_13160Hz.wav");
    }

    THEN("A missing sample has no path") {
      REQUIRE(assets::samplePath("s50", 3, root).empty());
      REQUIRE(assets::samplePath("s51", 1, root).empty());
    }
  }
}
