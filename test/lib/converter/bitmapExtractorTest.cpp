#include "../../../lib/converter/bitmapExtractor/bitmapExtractor.h"
#include "../../../lib/helpers/helpers.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::lib::converter::bitmapExtractor;
using openfranko::lib::helpers::pushBigEndian16;
using openfranko::lib::helpers::pushBigEndian32;

static std::vector<uint8_t> buildPackedBitmap(uint16_t height) {
  const uint32_t maskBytesOffset = 25;
  const uint32_t pointerBitsOffset = 26;
  std::vector<uint8_t> buf;
  pushBigEndian32(buf, 0x06071963);
  pushBigEndian32(buf, 0);
  pushBigEndian16(buf, 1);
  pushBigEndian16(buf, 1);
  pushBigEndian16(buf, height);
  pushBigEndian16(buf, 1);
  pushBigEndian32(buf, maskBytesOffset);
  pushBigEndian32(buf, pointerBitsOffset);
  buf.push_back(0x42);
  buf.push_back(0x00);
  buf.push_back(0x00);
  buf.push_back(0x00);
  return buf;
}

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

SCENARIO("extract says why it skipped a bitmap") {
  GIVEN("A multi-bitmap file with an 8x2 bitmap and an 8x1 bitmap") {
    auto data = buildPackedBitmap(2);
    auto tiny = buildPackedBitmap(1);
    data.insert(data.end(), tiny.begin(), tiny.end());

    WHEN("extract is called") {
      auto results = extract(data, "0388");
      REQUIRE(results.size() == 2);

      THEN("Both bitmaps are named by position") {
        REQUIRE(results[0].name == "0388");
        REQUIRE(results[1].name == "0388_1");
      }

      THEN("The 8x2 bitmap is converted to a BMP") {
        REQUIRE(results[0].error.empty());
        REQUIRE(results[0].bmpData.size() > 2);
        REQUIRE(results[0].bmpData[0] == 'B');
        REQUIRE(results[0].bmpData[1] == 'M');
      }

      THEN("The 8x1 bitmap is skipped with the reason") {
        REQUIRE(results[1].bmpData.empty());
        REQUIRE(results[1].error == "Bitmap is only 8x1 pixels");
      }
    }
  }

  GIVEN("File 0384 whose first bitmap is 8x1") {
    auto data = buildPackedBitmap(1);
    auto normal = buildPackedBitmap(2);
    data.insert(data.end(), normal.begin(), normal.end());

    WHEN("extract is called") {
      auto results = extract(data, "0384");
      REQUIRE(results.size() == 2);

      THEN("The skipped bitmap keeps its place in the numbering") {
        REQUIRE(results[0].name == "0384");
        REQUIRE(results[0].bmpData.empty());
        REQUIRE(results[0].error == "Bitmap is only 8x1 pixels");
        REQUIRE(results[1].name == "0384_1");
        REQUIRE(results[1].error.empty());
        REQUIRE_FALSE(results[1].bmpData.empty());
      }
    }
  }
}
