#include "../../../lib/converter/fileContainer/fileContainer.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::lib::converter::fileContainer;

SCENARIO("parseFooter reads the last 8 bytes as big-endian fields") {
  GIVEN("A 16-byte buffer with a known 8-byte suffix") {
    std::vector<uint8_t> data = {
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x11, 0x22,
        0x00, 0x01, 0x00, 0x00,
        0x03, 0x84,
        0x02, 0x00,
    };

    WHEN("parseFooter is called") {
      auto info = parseFooter(data);

      THEN("unpackSize is read correctly") { REQUIRE(info.unpackSize == 65536); }

      THEN("fileId is read correctly") { REQUIRE(info.fileId == 0x0384); }

      THEN("resourceType is read correctly") {
        REQUIRE(info.resourceType == 0x0200);
      }
    }
  }

  GIVEN("An exactly 8-byte buffer") {
    std::vector<uint8_t> data = {
        0x00, 0x00, 0x00, 0x42,
        0x00, 0x38,
        0x00, 0x00,
    };

    WHEN("parseFooter is called") {
      auto info = parseFooter(data);

      THEN("All fields are parsed from the entire buffer") {
        REQUIRE(info.unpackSize == 66);
        REQUIRE(info.fileId == 0x0038);
        REQUIRE(info.resourceType == 0x0000);
      }
    }
  }

  GIVEN("A buffer smaller than 8 bytes") {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};

    WHEN("parseFooter is called") {
      THEN("It throws a runtime_error") {
        REQUIRE_THROWS_AS(parseFooter(data), std::runtime_error);
      }
    }
  }
}
