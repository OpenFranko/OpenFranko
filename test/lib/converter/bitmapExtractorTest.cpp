#include "../../../lib/converter/bitmapExtractor/bitmapExtractor.h"
#include "../../../lib/helpers/helpers.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::lib::converter::bitmapExtractor;
using openfranko::lib::helpers::pushBigEndian16;
using openfranko::lib::helpers::pushBigEndian32;

static std::vector<uint8_t> buildPackedBitmap(uint16_t height,
                                              uint16_t planes = 1) {
  const uint32_t maskBytesOffset = 25;
  const uint32_t pointerBitsOffset = 26;
  std::vector<uint8_t> buf;
  pushBigEndian32(buf, 0x06071963);
  pushBigEndian32(buf, 0);
  pushBigEndian16(buf, 1);
  pushBigEndian16(buf, 1);
  pushBigEndian16(buf, height);
  pushBigEndian16(buf, planes);
  pushBigEndian32(buf, maskBytesOffset);
  pushBigEndian32(buf, pointerBitsOffset);
  buf.push_back(0x42);
  buf.push_back(0x00);
  buf.push_back(0x00);
  buf.push_back(0x00);
  return buf;
}

static std::vector<uint8_t>
buildOffsetTable(const std::vector<std::vector<uint8_t>> &images,
                 size_t entrySize) {
  std::vector<uint8_t> buf;
  size_t offset = images.size() * entrySize;
  for (const auto &image : images) {
    if (entrySize == 4) {
      pushBigEndian32(buf, static_cast<uint32_t>(offset));
    } else {
      pushBigEndian16(buf, static_cast<uint16_t>(offset));
    }
    offset += image.size();
  }
  for (const auto &image : images) {
    buf.insert(buf.end(), image.begin(), image.end());
  }
  return buf;
}

static std::vector<uint8_t>
buildTileFile(const std::vector<std::vector<uint8_t>> &tiles) {
  std::vector<uint8_t> buf;
  pushBigEndian16(buf, 0);
  buf.push_back(0x0B);
  buf.push_back(static_cast<uint8_t>(tiles.size()));
  for (const auto &tile : tiles) {
    pushBigEndian16(buf, static_cast<uint16_t>(tile.size() + 2));
    buf.insert(buf.end(), tile.begin(), tile.end());
  }
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
    auto data =
        buildOffsetTable({buildPackedBitmap(1), buildPackedBitmap(2)}, 2);

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

SCENARIO("extract skips a bitmap that fails to decode and keeps the rest") {
  GIVEN("A multi-bitmap file whose second bitmap has 7 bitplanes") {
    auto data = buildPackedBitmap(2);
    auto broken = buildPackedBitmap(2, 7);
    data.insert(data.end(), broken.begin(), broken.end());

    WHEN("extract is called") {
      auto results = extract(data, "0388");
      REQUIRE(results.size() == 2);

      THEN("The first bitmap is still converted") {
        REQUIRE(results[0].name == "0388");
        REQUIRE(results[0].error.empty());
        REQUIRE_FALSE(results[0].bmpData.empty());
      }

      THEN("The broken bitmap is skipped with the decoder's reason") {
        REQUIRE(results[1].name == "0388_1");
        REQUIRE(results[1].bmpData.empty());
        REQUIRE(results[1].error == "Unsupported bitplane count: 7");
      }
    }
  }

  GIVEN("A tile file whose second tile has 7 bitplanes") {
    auto data = buildTileFile({buildPackedBitmap(2), buildPackedBitmap(2, 7)});

    WHEN("extract is called") {
      auto results = extract(data, "0137");
      REQUIRE(results.size() == 2);

      THEN("The first tile is converted and the broken one is skipped") {
        REQUIRE(results[0].name == "0137_000");
        REQUIRE(results[0].error.empty());
        REQUIRE_FALSE(results[0].bmpData.empty());
        REQUIRE(results[1].name == "0137_001");
        REQUIRE(results[1].bmpData.empty());
        REQUIRE(results[1].error == "Unsupported bitplane count: 7");
      }
    }
  }
}

SCENARIO("extract finds bitmaps through the file's own tables") {
  GIVEN("A 32-bit offset table listing two bitmaps, then an unlisted one") {
    auto data =
        buildOffsetTable({buildPackedBitmap(2), buildPackedBitmap(2)}, 4);
    auto unlisted = buildPackedBitmap(2);
    data.insert(data.end(), unlisted.begin(), unlisted.end());

    WHEN("extract is called") {
      auto results = extract(data, "03BF");

      THEN("Only the listed bitmaps are extracted") {
        REQUIRE(results.size() == 2);
        REQUIRE(results[0].name == "03BF");
        REQUIRE(results[1].name == "03BF_1");
      }
    }
  }

  GIVEN("A 16-bit offset table with an entry pointing at non-bitmap data") {
    auto data = buildOffsetTable(
        {buildPackedBitmap(2), std::vector<uint8_t>(28, 0)}, 2);

    WHEN("extract is called") {
      auto results = extract(data, "03B7");
      REQUIRE(results.size() == 2);

      THEN("That entry is skipped with the reason") {
        REQUIRE(results[0].error.empty());
        REQUIRE(results[1].bmpData.empty());
        REQUIRE(results[1].error == "Invalid bitmap magic number");
      }
    }
  }

  GIVEN("A tile file with an unlisted bitmap between its two tiles") {
    auto tile = buildPackedBitmap(2);
    auto unlisted = buildPackedBitmap(2);
    std::vector<uint8_t> data;
    pushBigEndian16(data, 0);
    data.push_back(0x0B);
    data.push_back(2);
    pushBigEndian16(data,
                    static_cast<uint16_t>(tile.size() + unlisted.size() + 2));
    data.insert(data.end(), tile.begin(), tile.end());
    data.insert(data.end(), unlisted.begin(), unlisted.end());
    pushBigEndian16(data, static_cast<uint16_t>(tile.size() + 2));
    data.insert(data.end(), tile.begin(), tile.end());

    WHEN("extract is called") {
      auto results = extract(data, "0137");

      THEN("Only the tiles on the chain are extracted") {
        REQUIRE(results.size() == 2);
        REQUIRE(results[0].name == "0137_000");
        REQUIRE(results[1].name == "0137_001");
      }
    }
  }

  GIVEN("A tile file whose chain skips past its second tile") {
    auto data = buildTileFile({buildPackedBitmap(2), buildPackedBitmap(2)});
    data[5] = 10;

    WHEN("extract is called") {
      THEN("It throws, naming the broken tile") {
        REQUIRE_THROWS_WITH(extract(data, "0137"),
                            "Tile chain is broken at tile 1");
      }
    }
  }
}
