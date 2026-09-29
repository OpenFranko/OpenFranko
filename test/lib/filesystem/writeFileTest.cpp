#include "../../../lib/filesystem/writeFile/writeFile.h"

#include "../../../lib/filesystem/readFile/readFile.h"
#include "../../TemporaryPath.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
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
