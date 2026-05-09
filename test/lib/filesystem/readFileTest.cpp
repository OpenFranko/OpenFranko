#include "../../../lib/filesystem/readFile/readFile.h"
#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>

namespace {

void createTestFile(const std::string &filePath,
                    const std::vector<uint8_t> &data) {
  std::ofstream outFile(filePath, std::ios::binary);
  outFile.write(reinterpret_cast<const char *>(data.data()), data.size());
  outFile.close();
}

} // namespace

SCENARIO("ReadFile works correctly") {
  GIVEN("A valid text file") {
    std::string filePath = "./test.txt";

    std::vector<uint8_t> expectedData = {'T', 'e', 's', 't', ' ',
                                         'd', 'a', 't', 'a'};

    createTestFile(filePath, expectedData);

    WHEN("Reading the file") {
      const auto actualData =
          openfranko::lib::filesystem::readFile::readFile(filePath);
      THEN("The file data should match the expected output") {
        REQUIRE(actualData == expectedData);

        std::filesystem::remove(filePath);
      }
    }
  }
}