#include "../../../../src/engine/assets/DiskFiles.h"

#include "../../../TemporaryPath.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace openfranko::src::engine::assets;
using namespace openfranko::test;

namespace {

void write(const std::filesystem::path &path, const std::string &text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream(path, std::ios::binary) << text;
}

} // namespace

SCENARIO("DiskFiles reads the extracted files from disk") {
  GIVEN("A bank directory with two samples") {
    const TemporaryPath directory("openFrankoDiskFiles");
    const std::string bank = (directory.path() / "0263").string();
    const std::string first = bank + "/0263_sam1_13160Hz.wav";
    const std::string second = bank + "/0263_sam2_8363Hz.wav";
    const std::string missing = bank + "/0263_sam3_8363Hz.wav";
    write(first, "RIFF");
    write(second, "WAVE");
    DiskFiles files;

    THEN("They exist and a third does not") {
      REQUIRE(files.exists(first));
      REQUIRE(files.exists(second));
      REQUIRE_FALSE(files.exists(missing));
    }

    THEN("The bank lists both, and a missing bank nothing") {
      std::vector<std::string> listed = files.list(bank);
      std::sort(listed.begin(), listed.end());
      REQUIRE(listed == std::vector<std::string>{first, second});
      REQUIRE(files.list((directory.path() / "0264").string()).empty());
    }

    THEN("A file is read whole") {
      REQUIRE(files.read(first) == std::vector<uint8_t>{'R', 'I', 'F', 'F'});
    }

    THEN("A missing file is refused with its path") {
      REQUIRE_THROWS_WITH(files.read(missing),
                          Catch::Matchers::ContainsSubstring(missing));
      REQUIRE_THROWS_WITH(files.loadBitmap(missing),
                          Catch::Matchers::ContainsSubstring(missing));
    }
  }
}
