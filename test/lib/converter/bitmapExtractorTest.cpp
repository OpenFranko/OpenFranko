#include "../../../lib/converter/bitmapExtractor/bitmapExtractor.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::lib::converter::bitmapExtractor;

SCENARIO("extract handles edge cases gracefully") {
  GIVEN("Data smaller than 4 bytes") {
    std::vector<uint8_t> tiny = {0x01, 0x02};

    WHEN("extract is called") {
      THEN("It throws a runtime error") {
        REQUIRE_THROWS_AS(extract(tiny, "0100"), std::runtime_error);
      }
    }
  }

  GIVEN("An empty data vector") {
    std::vector<uint8_t> empty;

    WHEN("extract is called") {
      THEN("It throws a runtime error") {
        REQUIRE_THROWS_AS(extract(empty, "0100"), std::runtime_error);
      }
    }
  }

  GIVEN("4 bytes of data with no valid magic number") {
    std::vector<uint8_t> garbage = {0xDE, 0xAD, 0xBE, 0xEF};

    WHEN("extract is called with a non-tile file ID") {
      auto results = extract(garbage, "03B7");

      THEN("It returns an empty vector") { REQUIRE(results.empty()); }
    }
  }

  GIVEN("4 bytes of data with no valid magic number") {
    std::vector<uint8_t> garbage = {0xDE, 0xAD, 0xBE, 0xEF};

    WHEN("extract is called with a tile file ID") {
      THEN("It throws a runtime error") {
        REQUIRE_THROWS_AS(extract(garbage, "0137"), std::runtime_error);
      }
    }
  }
}

SCENARIO("extract dispatches 0384 to extract0384 path") {
  GIVEN("Data that starts with non-SPACK, non-BMCode magic for fileId 0384") {
    std::vector<uint8_t> data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    WHEN("extract is called with fileId '0384'") {
      auto results = extract(data, "0384");

      THEN("It returns empty (no valid images found)") {
        REQUIRE(results.empty());
      }
    }
  }
}

SCENARIO("extract dispatches tile file IDs to extractTiles") {
  GIVEN("Insufficient data for any tile file") {
    std::vector<uint8_t> data(30, 0);

    WHEN("extract is called with various tile file IDs") {
      THEN("All known tile IDs throw a runtime error") {
        for (const auto &id :
             {"0137", "0138", "0139", "013A", "013B", "013C", "013D", "013E",
              "013F", "0140", "0141", "0142", "0143", "0144", "0145", "014A",
              "0154", "014B", "014C", "014D", "014E", "014F"}) {
          REQUIRE_THROWS_AS(extract(data, id), std::runtime_error);
        }
      }
    }
  }
}
