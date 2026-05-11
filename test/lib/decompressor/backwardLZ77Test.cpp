#include "../../../lib/decompressor/backwardLZ77/backwardLZ77.h"
#include <catch2/catch_all.hpp>

using namespace openfranko::lib::decompressor::backwardLZ77;

SCENARIO("BackwardLZ77 decompression works correctly") {
  GIVEN("A short literal run encoding a single byte") {
    std::vector<uint8_t> compressedData = {
        0x00, 0x00, 0x28, 0x40, 0x00, 0x00, 0x28, 0x40, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected = {0x42};
        REQUIRE(decompressedData == expected);
      }
    }
  }

  GIVEN("A short literal run encoding 5 bytes") {
    std::vector<uint8_t> compressedData = {
        0xF2, 0x32, 0x32, 0xA2, 0x00, 0x00, 0x22, 0x44, 0xF2, 0x32, 0x10, 0xE6,
        0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected = {0x4F, 0x4C, 0x4C, 0x45, 0x48};
        REQUIRE(decompressedData == expected);
      }
    }
  }

  GIVEN("A simple short match with 8-bit offset and 2 copies") {
    std::vector<uint8_t> compressedData = {
        0xA0, 0x4A, 0xBB, 0xB0, 0xA0, 0x4A, 0xBB, 0xB0, 0x00, 0x00,
        0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected = {0xAA, 0xBB, 0xAA, 0xBB};
        REQUIRE(decompressedData == expected);
      }
    }
  }

  GIVEN("A complex short match type 0 with 9-bit offset and 3 copies") {
    std::vector<uint8_t> compressedData = {
        0x03, 0x00, 0x30, 0x40, 0x03, 0x00, 0x30, 0x40, 0x00, 0x00,
        0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected = {0x41, 0x41, 0x41, 0x41};
        REQUIRE(decompressedData == expected);
      }
    }
  }

  GIVEN("A complex short match type 1 with 10-bit offset and 4 copies") {
    std::vector<uint8_t> compressedData = {
        0x40, 0x2C, 0x12, 0x14, 0x00, 0x00, 0x00, 0x04, 0x40, 0x2C, 0x12, 0x10,
        0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected = {0x41, 0x42, 0x41, 0x42, 0x41, 0x42};
        REQUIRE(decompressedData == expected);
      }
    }
  }

  GIVEN("A long match type 2 with 8-bit length and 12-bit offset") {
    std::vector<uint8_t> compressedData = {
        0x80, 0x0A, 0x06, 0x34, 0x00, 0x00, 0x00, 0x10, 0x80, 0x0A, 0x06, 0x24,
        0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected(7, 0x58);
        REQUIRE(decompressedData == expected);
      }
    }
  }

  GIVEN("A long literal run encoding 9 bytes") {
    std::vector<uint8_t> compressedData = {
        0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x00, 0x0C,
        0x90, 0x07, 0x20, 0x2C, 0xB0, 0x37, 0x00, 0x00, 0x00, 0x09,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected = {0x41, 0x42, 0x43, 0x44, 0x45,
                                         0x46, 0x47, 0x48, 0x49};
        REQUIRE(decompressedData == expected);
      }
    }
  }

  GIVEN("A long literal run encoding 12 bytes") {
    std::vector<uint8_t> compressedData = {
        0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C,
        0xDC, 0x00, 0x00, 0x0E, 0x07, 0x3C, 0xBC, 0x72, 0xFB, 0x00, 0x00,
        0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
                                         0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B};
        REQUIRE(decompressedData == expected);
      }
    }
  }

  GIVEN("Mixed commands: literals then match then literals") {
    std::vector<uint8_t> compressedData = {
        0x77, 0xFF, 0x81, 0x04, 0xAB, 0xBA, 0x67, 0x77, 0x00, 0x00,
        0x00, 0x18, 0xDC, 0x45, 0xE6, 0x6B, 0x00, 0x00, 0x00, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected = {0xEE, 0xFF, 0xCC, 0xDD,
                                         0xAA, 0xBB, 0xCC, 0xDD};
        REQUIRE(decompressedData == expected);
      }
    }
  }

  GIVEN("Multiple matches chained together") {
    std::vector<uint8_t> compressedData = {
        0xA0, 0x90, 0x06, 0x01, 0x00, 0x00, 0x08, 0x10, 0xA0, 0x90, 0x0E, 0x11,
        0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected = {0x01, 0x02, 0x02, 0x01,
                                         0x02, 0x01, 0x02};
        REQUIRE(decompressedData == expected);
      }
    }
  }

  GIVEN("A long match with a larger length field") {
    std::vector<uint8_t> compressedData = {
        0x40, 0x09, 0x07, 0x67, 0x00, 0x00, 0x1A, 0xB0, 0x40, 0x09, 0x1D, 0xD7,
        0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The decompressed data should match the expected output") {
        std::vector<uint8_t> expected = {0xCD, 0xAB, 0xCD, 0xAB, 0xCD, 0xAB,
                                         0xCD, 0xAB, 0xCD, 0xAB, 0xCD, 0xAB};
        REQUIRE(decompressedData == expected);
      }
    }
  }

  GIVEN("Compressed data with unpackedSize of 0") {
    std::vector<uint8_t> compressedData = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Decompressing the data") {
      const auto decompressedData = decompress(compressedData);

      THEN("The result should be empty") { REQUIRE(decompressedData.empty()); }
    }
  }

  GIVEN("Compressed data that is too small to contain a footer") {
    std::vector<uint8_t> compressedData = {0x00, 0x01, 0x02};

    WHEN("Attempting to decompress") {
      THEN("It should throw a runtime error") {
        REQUIRE_THROWS_AS(decompress(compressedData), std::runtime_error);
      }
    }
  }

  GIVEN("Compressed data with a corrupted checksum") {
    std::vector<uint8_t> compressedData = {
        0x00, 0x00, 0x28, 0x40, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    WHEN("Attempting to decompress") {
      THEN("It should throw a runtime error") {
        REQUIRE_THROWS_AS(decompress(compressedData), std::runtime_error);
      }
    }
  }
}
