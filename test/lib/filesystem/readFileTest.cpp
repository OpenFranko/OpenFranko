#include "../../../lib/filesystem/readFile/readFile.h"

#include "../../TemporaryPath.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
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
