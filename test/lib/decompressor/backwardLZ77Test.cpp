#include "../../../lib/decompressor/backwardLZ77/backwardLZ77.h"
#include <catch2/catch_all.hpp>

SCENARIO("BackwardLZ77 decompression works correctly") {
  GIVEN("Test BackwardLZ77 compressed data") {

    std::vector<uint8_t> compressedData = {
        0x54, 0x45, 0x53, 0x54, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x04,
        0x46, 0x52, 0x41, 0x4E, 0x4B, 0x4F, 0x30, 0x31};

    WHEN("Decompressing the data") {

      const auto decompressedData =
          openfranko::lib::decompressor::backwardLZ77::decompress(
              compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected = {'T', 'E', 'S', 'T'};

        REQUIRE(decompressedData == expected);
      }
    }
  }
}