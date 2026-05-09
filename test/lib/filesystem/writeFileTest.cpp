#include "../../../lib/filesystem/writeFile/writeFile.h"
#include "../../../lib/filesystem/readFile/readFile.h"
#include <catch2/catch_all.hpp>
#include <filesystem>

SCENARIO("WriteFile works correctly") {
  GIVEN("A valid text file") {
    std::string filePath = "./test.txt";

    std::vector<uint8_t> expectedData = {'T', 'e', 's', 't', ' ',
                                         'd', 'a', 't', 'a'};

    WHEN("Writing the file") {
      openfranko::lib::filesystem::writeFile::writeFile(filePath, expectedData);

      THEN("The file data should match the expected output") {
        const auto actualData =
            openfranko::lib::filesystem::readFile::readFile(filePath);
        REQUIRE(actualData == expectedData);

        std::filesystem::remove(filePath);
      }
    }
  }
}