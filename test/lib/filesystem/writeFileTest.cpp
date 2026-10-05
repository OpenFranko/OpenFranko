#include "../../../lib/filesystem/writeFile/writeFile.h"

#include "../../../lib/filesystem/readFile/readFile.h"
#include "../../TemporaryPath.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

using namespace openfranko::lib::filesystem::readFile;
using namespace openfranko::lib::filesystem::writeFile;
using namespace openfranko::test;

SCENARIO("writeFile stores the bytes it is given") {
  GIVEN("A valid text file") {
    const TemporaryPath file("openFrankoWriteFile.txt");

    std::vector<uint8_t> expectedData = {'T', 'e', 's', 't', ' ',
                                         'd', 'a', 't', 'a'};

    WHEN("Writing the file") {
      writeFile(file.path().string(), expectedData);

      THEN("The file data matches the expected output") {
        const auto actualData = readFile(file.path().string());
        REQUIRE(actualData == expectedData);
      }
    }
  }
}

SCENARIO("writeFile refuses paths it cannot create") {
  GIVEN("A file in a directory that does not exist") {
    const TemporaryPath directory("openFrankoWriteFileMissing");
    const std::string path = (directory.path() / "out.bin").string();

    WHEN("Writing the file") {
      THEN("It throws, naming the file, and creates nothing") {
        REQUIRE_THROWS_WITH(writeFile(path, {1, 2, 3}),
                            "Cannot open file for writing: " + path);
        REQUIRE_FALSE(std::filesystem::exists(directory.path()));
      }
    }
  }

  GIVEN("An existing directory") {
    const TemporaryPath directory("openFrankoWriteFileDirectory");
    std::filesystem::create_directories(directory.path());
    const std::string path = directory.path().string();

    WHEN("Writing to it as a file") {
      THEN("It throws, naming the path, and the directory stays") {
        REQUIRE_THROWS_WITH(writeFile(path, {1, 2, 3}),
                            "Cannot open file for writing: " + path);
        REQUIRE(std::filesystem::is_directory(directory.path()));
      }
    }
  }
}
