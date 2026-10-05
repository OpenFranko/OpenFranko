#include "../../../../src/engine/assets/GameFiles.h"

#include "../TemporaryWorkingDirectory.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace openfranko::src::engine::assets;
using namespace openfranko::test::src::engine;

namespace {

constexpr auto CREDITS = "assets/credits.json";

std::string tarArchive(const std::string &name, const std::string &contents) {
  constexpr std::size_t BLOCK_SIZE = 512;
  std::string header(BLOCK_SIZE, '\0');
  header.replace(0, name.size(), name);
  char length[12];
  std::snprintf(length, sizeof(length), "%011zo", contents.size());
  header.replace(124, 11, length);
  header[156] = '0';
  header.replace(257, 5, "ustar");
  const std::size_t padding =
      (BLOCK_SIZE - contents.size() % BLOCK_SIZE) % BLOCK_SIZE;
  return header + contents + std::string(padding, '\0') +
         std::string(2 * BLOCK_SIZE, '\0');
}

std::string readText(Files &files, const std::string &path) {
  const std::vector<uint8_t> bytes = files.read(path);
  return std::string(bytes.begin(), bytes.end());
}

} // namespace

SCENARIO("The game files come from assets.tar before the assets directory") {
  GIVEN("A working directory with both assets.tar and an assets directory") {
    const TemporaryWorkingDirectory directory("openFrankoGameFilesBoth");
    writeFile("assets.tar", tarArchive(CREDITS, "from the archive"));
    writeFile(CREDITS, "from the directory");

    THEN("The archive is read") {
      const std::unique_ptr<Files> files = openGameFiles();
      REQUIRE(readText(*files, CREDITS) == "from the archive");
    }
  }

  GIVEN("A working directory with only an assets directory") {
    const TemporaryWorkingDirectory directory("openFrankoGameFilesDirectory");
    writeFile(CREDITS, "from the directory");

    THEN("The directory is read") {
      const std::unique_ptr<Files> files = openGameFiles();
      REQUIRE(files->exists("assets"));
      REQUIRE(readText(*files, CREDITS) == "from the directory");
    }
  }

  GIVEN("A working directory without game data") {
    const TemporaryWorkingDirectory directory("openFrankoGameFilesNone");

    THEN("The files are looked for on disk and none exist") {
      const std::unique_ptr<Files> files = openGameFiles();
      REQUIRE_FALSE(files->exists("assets"));
      REQUIRE_THROWS_WITH(files->read(CREDITS),
                          Catch::Matchers::ContainsSubstring(CREDITS));
    }
  }
}
