#include "../../../../src/engine/assets/ArchiveFiles.h"

#include "../../../TemporaryPath.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace openfranko::src::engine::assets;
using namespace openfranko::test;

namespace {

constexpr std::size_t BLOCK_SIZE = 512;
constexpr std::size_t LENGTH_OFFSET = 124;
constexpr std::size_t TYPE_OFFSET = 156;
constexpr std::size_t MAGIC_OFFSET = 257;
constexpr std::size_t PREFIX_OFFSET = 345;
constexpr char DIRECTORY_TYPE = '5';

struct ArchiveEntry {
  std::string name;
  std::string contents;
  char type = '0';
  std::string prefix;
};

void put(std::vector<char> &block, std::size_t offset,
         const std::string &text) {
  std::copy(text.begin(), text.end(),
            block.begin() + static_cast<std::ptrdiff_t>(offset));
}

std::string octal(std::size_t value) {
  char digits[12];
  std::snprintf(digits, sizeof(digits), "%011zo", value);
  return digits;
}

void writeArchive(const std::filesystem::path &path,
                  const std::vector<ArchiveEntry> &entries,
                  const std::string &magic = "ustar") {
  std::ofstream file(path, std::ios::binary);
  for (const ArchiveEntry &entry : entries) {
    std::vector<char> header(BLOCK_SIZE, '\0');
    put(header, 0, entry.name);
    put(header, LENGTH_OFFSET, octal(entry.contents.size()));
    header[TYPE_OFFSET] = entry.type;
    put(header, MAGIC_OFFSET, magic);
    put(header, PREFIX_OFFSET, entry.prefix);
    file.write(header.data(), static_cast<std::streamsize>(header.size()));
    file << entry.contents
         << std::string((BLOCK_SIZE - entry.contents.size() % BLOCK_SIZE) %
                            BLOCK_SIZE,
                        '\0');
  }
  file << std::string(2 * BLOCK_SIZE, '\0');
}

} // namespace

SCENARIO("ArchiveFiles reads the extracted files from a tar archive") {
  GIVEN("An archive of an assets directory with a bank of two samples") {
    const TemporaryPath archive("openFrankoArchiveFiles.tar");
    const std::string first = "assets/0263/0263_sam1_13160Hz.wav";
    const std::string second = "assets/0263/0263_sam2_8363Hz.wav";
    const std::string missing = "assets/0263/0263_sam3_8363Hz.wav";
    const std::string picture = "assets/03B6.bmp";
    writeArchive(archive.path(), {{"./assets/", "", DIRECTORY_TYPE},
                                  {"./assets/0263/", "", DIRECTORY_TYPE},
                                  {"./" + first, "RIFF"},
                                  {"./" + second, std::string(600, 'W')},
                                  {"./" + picture, "XX"}});
    ArchiveFiles files(archive.path().string());

    THEN("The files and their directories exist and a third sample does not") {
      REQUIRE(files.exists(first));
      REQUIRE(files.exists(second));
      REQUIRE(files.exists("assets/0263"));
      REQUIRE(files.exists("assets"));
      REQUIRE_FALSE(files.exists(missing));
      REQUIRE_FALSE(files.exists("assets/02"));
    }

    THEN("A directory lists its files and subdirectories") {
      REQUIRE(files.list("assets") ==
              std::vector<std::string>{"assets/0263", picture});
      REQUIRE(files.list("assets/0263") ==
              std::vector<std::string>{first, second});
      REQUIRE(files.list("assets/0264").empty());
    }

    THEN("A file is read whole, across blocks") {
      REQUIRE(files.read(first) == std::vector<uint8_t>{'R', 'I', 'F', 'F'});
      REQUIRE(files.read(second) == std::vector<uint8_t>(600, 'W'));
    }

    THEN("A missing file is refused with its path") {
      REQUIRE_THROWS_WITH(files.read(missing),
                          Catch::Matchers::ContainsSubstring(missing));
      REQUIRE_THROWS_WITH(files.loadBitmap(missing),
                          Catch::Matchers::ContainsSubstring(missing));
    }

    THEN("A file that is no bitmap is refused with its path") {
      REQUIRE_THROWS_WITH(files.loadBitmap(picture),
                          Catch::Matchers::ContainsSubstring(picture));
    }
  }

  GIVEN("An entry whose directory is kept in the ustar prefix") {
    const TemporaryPath archive("openFrankoArchivePrefix.tar");
    writeArchive(archive.path(), {{"0137_000.bmp", "BM", '0', "assets/0137"}});
    ArchiveFiles files(archive.path().string());

    THEN("It is found under the joined path") {
      REQUIRE(files.read("assets/0137/0137_000.bmp") ==
              std::vector<uint8_t>{'B', 'M'});
    }
  }
}

SCENARIO("ArchiveFiles refuses what is not a tar archive") {
  GIVEN("A path with no file") {
    const TemporaryPath missing("openFrankoMissingArchive.tar");

    THEN("The error carries the path") {
      REQUIRE_THROWS_WITH(
          ArchiveFiles(missing.path().string()),
          Catch::Matchers::ContainsSubstring(missing.path().string()));
    }
  }

  GIVEN("A file without the ustar magic") {
    const TemporaryPath archive("openFrankoNotArchive.tar");
    writeArchive(archive.path(), {{"assets/03B6.bmp", "BM"}}, "zip");

    THEN("It is refused") {
      REQUIRE_THROWS_AS(ArchiveFiles(archive.path().string()),
                        std::runtime_error);
    }
  }
}
