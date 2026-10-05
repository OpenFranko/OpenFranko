#include "../../../lib/filesystem/readFile/readFile.h"

#include "../../TemporaryPath.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace openfranko::lib::filesystem::readFile;
using namespace openfranko::test;

namespace {

void createTestFile(const std::filesystem::path &filePath,
                    const std::vector<uint8_t> &data) {
  std::ofstream outFile(filePath, std::ios::binary);
  outFile.write(reinterpret_cast<const char *>(data.data()), data.size());
  outFile.close();
}

} // namespace

SCENARIO("readFile returns the bytes of a file") {
  GIVEN("A valid text file") {
    const TemporaryPath file("openFrankoReadFile.txt");

    std::vector<uint8_t> expectedData = {'T', 'e', 's', 't', ' ',
                                         'd', 'a', 't', 'a'};

    createTestFile(file.path(), expectedData);

    WHEN("Reading the file") {
      const auto actualData = readFile(file.path().string());
      THEN("The file data matches the expected output") {
        REQUIRE(actualData == expectedData);
      }
    }
  }
}

SCENARIO("readFile refuses paths it cannot read") {
  GIVEN("A file in a directory that does not exist") {
    const TemporaryPath directory("openFrankoReadFileMissing");
    const std::string path = (directory.path() / "missing.bin").string();

    WHEN("Reading the file") {
      THEN("It throws, naming the file") {
        REQUIRE_THROWS_WITH(readFile(path),
                            "Cannot open file for reading: " + path);
      }
    }
  }

  GIVEN("A directory") {
    const TemporaryPath directory("openFrankoReadFileDirectory");
    std::filesystem::create_directories(directory.path());

    WHEN("Reading it as a file") {
      THEN("It throws instead of returning data") {
        REQUIRE_THROWS_AS(readFile(directory.path().string()),
                          std::runtime_error);
      }
    }
  }
}
