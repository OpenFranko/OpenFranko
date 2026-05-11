#include "../../../lib/decompressor/amosCompact/amosCompact.h"
#include "../../../lib/decompressor/amosCompact/BitReader.h"
#include "../../../lib/decompressor/amosCompact/ByteReader.h"
#include "../../../lib/decompressor/amosCompact/Consts.h"
#include "../../../lib/decompressor/amosCompact/headers.h"
#include "../../../lib/shared/unpackedBitmap.h"
#include <catch2/catch_all.hpp>
#include <cstdlib>
#include <vector>

using namespace openfranko::lib::decompressor::amosCompact;

static void freeBitmap(UnpackedBitmap &bmp) {
  for (size_t p = 0; p < consts::MAX_SUPPORTED_BITPLANES; p++) {
    free(bmp.bitplaneData[p]);
    bmp.bitplaneData[p] = nullptr;
  }
  free(bmp.chunkyPixels);
  bmp.chunkyPixels = nullptr;
}

static std::vector<uint8_t>
buildPackedBitmap(uint16_t tx, uint16_t ty, uint16_t tcar, uint16_t nplan,
                  const std::vector<uint8_t> &dataStream1,
                  const std::vector<uint8_t> &dataStream2,
                  const std::vector<uint8_t> &pointerBitstream) {
  uint32_t datas2Off = 24 + static_cast<uint32_t>(dataStream1.size());
  uint32_t pointOff = datas2Off + static_cast<uint32_t>(dataStream2.size());

  std::vector<uint8_t> data = {
      0x06,
      0x07,
      0x19,
      0x63,
      0x00,
      0x00,
      0x00,
      0x00,
      static_cast<uint8_t>(tx >> 8),
      static_cast<uint8_t>(tx & 0xFF),
      static_cast<uint8_t>(ty >> 8),
      static_cast<uint8_t>(ty & 0xFF),
      static_cast<uint8_t>(tcar >> 8),
      static_cast<uint8_t>(tcar & 0xFF),
      static_cast<uint8_t>(nplan >> 8),
      static_cast<uint8_t>(nplan & 0xFF),
      static_cast<uint8_t>(datas2Off >> 24),
      static_cast<uint8_t>(datas2Off >> 16),
      static_cast<uint8_t>(datas2Off >> 8),
      static_cast<uint8_t>(datas2Off & 0xFF),
      static_cast<uint8_t>(pointOff >> 24),
      static_cast<uint8_t>(pointOff >> 16),
      static_cast<uint8_t>(pointOff >> 8),
      static_cast<uint8_t>(pointOff & 0xFF),
  };

  data.insert(data.end(), dataStream1.begin(), dataStream1.end());
  data.insert(data.end(), dataStream2.begin(), dataStream2.end());
  data.insert(data.end(), pointerBitstream.begin(), pointerBitstream.end());

  return data;
}

SCENARIO("BitReader reads bits MSB-first from a byte stream") {
  GIVEN("A single byte 0xA5 (10100101)") {
    std::vector<uint8_t> data = {0xA5};
    BitReader reader(data, 0);

    WHEN("Reading all 8 bits") {
      std::vector<int> bits;
      for (int i = 0; i < 8; i++) {
        bits.push_back(reader.read());
      }

      THEN("They match the MSB-first bit pattern") {
        REQUIRE(bits == std::vector<int>{1, 0, 1, 0, 0, 1, 0, 1});
      }
    }
  }

  GIVEN("Two bytes 0xA5 0x3C") {
    std::vector<uint8_t> data = {0xA5, 0x3C};
    BitReader reader(data, 0);

    WHEN("Reading 16 bits across the byte boundary") {
      std::vector<int> bits;
      for (int i = 0; i < 16; i++) {
        bits.push_back(reader.read());
      }

      THEN("All 16 bits are correct") {
        REQUIRE(bits == std::vector<int>{1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1,
                                         1, 0, 0});
      }
    }
  }

  GIVEN("A single byte") {
    std::vector<uint8_t> data = {0x80};
    BitReader reader(data, 0);

    WHEN("Reading past the end") {
      for (int i = 0; i < 8; i++)
        reader.read();

      THEN("Extra reads return 0") {
        REQUIRE(reader.read() == 0);
        REQUIRE(reader.read() == 0);
      }
    }
  }

  GIVEN("An offset into the data") {
    std::vector<uint8_t> data = {0xFF, 0xA5};
    BitReader reader(data, 1);

    WHEN("Reading bits from the offset") {
      std::vector<int> bits;
      for (int i = 0; i < 8; i++) {
        bits.push_back(reader.read());
      }

      THEN("They come from the second byte") {
        REQUIRE(bits == std::vector<int>{1, 0, 1, 0, 0, 1, 0, 1});
      }
    }
  }
}

SCENARIO("ByteReader reads bytes sequentially") {
  GIVEN("Three bytes") {
    std::vector<uint8_t> data = {0x41, 0x42, 0x43};
    ByteReader reader(data, 0);

    WHEN("Reading them in order") {
      THEN("Each read returns the next byte") {
        REQUIRE(reader.read() == 0x41);
        REQUIRE(reader.read() == 0x42);
        REQUIRE(reader.read() == 0x43);
      }
    }
  }

  GIVEN("A single byte") {
    std::vector<uint8_t> data = {0x41};
    ByteReader reader(data, 0);

    WHEN("Reading past the end") {
      reader.read();

      THEN("Extra reads return 0") {
        REQUIRE(reader.read() == 0);
        REQUIRE(reader.read() == 0);
      }
    }
  }

  GIVEN("An offset into the data") {
    std::vector<uint8_t> data = {0xFF, 0x42, 0x43};
    ByteReader reader(data, 1);

    WHEN("Reading bytes from the offset") {
      THEN("They start from the second byte") {
        REQUIRE(reader.read() == 0x42);
        REQUIRE(reader.read() == 0x43);
      }
    }
  }
}

SCENARIO("SPACK header parsing extracts all fields correctly") {
  GIVEN("A 90-byte SPACK header with known values") {
    std::vector<uint8_t> data(90, 0);
    data[0] = 0x12;
    data[1] = 0x03;
    data[2] = 0x19;
    data[3] = 0x90;
    data[4] = 0x01;
    data[5] = 0x40;
    data[6] = 0x00;
    data[7] = 0xC8;
    data[8] = 0x00;
    data[9] = 0x10;
    data[10] = 0x00;
    data[11] = 0x20;
    data[12] = 0x01;
    data[13] = 0x30;
    data[14] = 0x00;
    data[15] = 0xB8;
    data[16] = 0x00;
    data[17] = 0x05;
    data[18] = 0x00;
    data[19] = 0x0A;
    data[20] = 0x80;
    data[21] = 0x00;
    data[22] = 0x00;
    data[23] = 0x10;
    data[24] = 0x00;
    data[25] = 0x04;
    data[28] = 0x0F;
    data[29] = 0x00;

    WHEN("Parsing the header") {
      auto hdr = headers::parseSPACKHeader(data);

      THEN("All fields are correct") {
        REQUIRE(hdr.screenWidth == 320);
        REQUIRE(hdr.screenHeight == 200);
        REQUIRE(hdr.windowX == 16);
        REQUIRE(hdr.windowY == 32);
        REQUIRE(hdr.windowWidth == 304);
        REQUIRE(hdr.windowHeight == 184);
        REQUIRE(hdr.viewX == 5);
        REQUIRE(hdr.viewY == 10);
        REQUIRE(hdr.displayModeFlags == 0x8000);
        REQUIRE(hdr.numberOfColors == 16);
        REQUIRE(hdr.numberOfBitplanes == 4);
        REQUIRE(hdr.amigaPalette[0] == 0x000);
        REQUIRE(hdr.amigaPalette[1] == 0xF00);
      }
    }
  }
}

SCENARIO("Bitmap header parsing extracts all fields correctly") {
  GIVEN("A 24-byte bitmap header with known values") {
    std::vector<uint8_t> data = {
        0x06, 0x07, 0x19, 0x63, 0xFF, 0xFE, 0x00, 0x03, 0x00, 0x28, 0x00, 0x0A,
        0x00, 0x10, 0x00, 0x04, 0x00, 0x00, 0x12, 0x34, 0x00, 0x00, 0x56, 0x78,
    };

    WHEN("Parsing the header") {
      auto hdr = headers::parseBitmapHeader(data);

      THEN("All fields are correct including signed offsets") {
        REQUIRE(hdr.xOffset == -2);
        REQUIRE(hdr.yOffset == 3);
        REQUIRE(hdr.gridX == 40);
        REQUIRE(hdr.gridY == 10);
        REQUIRE(hdr.tileHeight == 16);
        REQUIRE(hdr.numberOfBitplanes == 4);
        REQUIRE(hdr.offsetToByteTable2 == 0x1234);
        REQUIRE(hdr.offsetToPointerBitstream == 0x5678);
      }
    }
  }
}

SCENARIO("AMOS Compact decompression works correctly") {
  GIVEN("A minimal single-cell bitmap with mask=0 (RLE, no fresh reads)") {
    auto data = buildPackedBitmap(1, 1, 1, 1, {0x42}, {0x00}, {0x00});

    WHEN("Decompressing") {
      auto result = decompress(data);

      THEN("Output has correct dimensions") {
        REQUIRE(result.width == 8);
        REQUIRE(result.height == 1);
        REQUIRE(result.numberOfBitplanes == 1);
      }

      THEN("Plane data contains the initial value") {
        REQUIRE(result.bitplaneData[0][0] == 0x42);
      }

      THEN("Chunky pixels reflect the bitplane data") {
        std::vector<uint8_t> expected = {0, 1, 0, 0, 0, 0, 1, 0};
        for (int i = 0; i < 8; i++) {
          REQUIRE(result.chunkyPixels[i] == expected[i]);
        }
      }

      freeBitmap(result);
    }
  }

  GIVEN("A single-cell bitmap where a mask bit triggers a fresh value read") {
    auto data = buildPackedBitmap(1, 1, 1, 1, {0x00, 0xFF}, {0x80}, {0x00});

    WHEN("Decompressing") {
      auto result = decompress(data);

      THEN("The fresh value overwrites the initial") {
        REQUIRE(result.bitplaneData[0][0] == 0xFF);
      }

      freeBitmap(result);
    }
  }

  GIVEN("A multi-cell bitmap with mask=0 repeating the same value (RLE)") {
    auto data = buildPackedBitmap(3, 1, 1, 1, {0xAA}, {0x00}, {0x00});

    WHEN("Decompressing") {
      auto result = decompress(data);

      THEN("All plane bytes are the repeated value") {
        REQUIRE(result.width == 24);
        REQUIRE(result.height == 1);
        for (int i = 0; i < 3; i++) {
          REQUIRE(result.bitplaneData[0][i] == 0xAA);
        }
      }

      freeBitmap(result);
    }
  }

  GIVEN("A 2x2 tile grid with tileHeight=2 verifying tile traversal order") {
    auto data = buildPackedBitmap(
        2, 2, 2, 1, {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
        {0xFF}, {0x00});

    WHEN("Decompressing") {
      auto result = decompress(data);

      THEN("Dimensions are correct") {
        REQUIRE(result.width == 16);
        REQUIRE(result.height == 4);
      }

      THEN("Tile traversal produces correct row-major plane layout") {
        REQUIRE(result.bitplaneData[0][0] == 0x01);
        REQUIRE(result.bitplaneData[0][1] == 0x03);
        REQUIRE(result.bitplaneData[0][2] == 0x02);
        REQUIRE(result.bitplaneData[0][3] == 0x04);
        REQUIRE(result.bitplaneData[0][4] == 0x05);
        REQUIRE(result.bitplaneData[0][5] == 0x07);
        REQUIRE(result.bitplaneData[0][6] == 0x06);
        REQUIRE(result.bitplaneData[0][7] == 0x08);
      }

      freeBitmap(result);
    }
  }

  GIVEN("A two-plane bitmap combining into chunky pixels") {
    auto data =
        buildPackedBitmap(1, 1, 1, 2, {0x00, 0xAA, 0x55}, {0xFF}, {0x00});

    WHEN("Decompressing") {
      auto result = decompress(data);

      THEN("Each plane has its own data") {
        REQUIRE(result.numberOfBitplanes == 2);
        REQUIRE(result.bitplaneData[0][0] == 0xAA);
        REQUIRE(result.bitplaneData[1][0] == 0x55);
      }

      THEN("Chunky pixels combine both planes") {
        std::vector<uint8_t> expected = {1, 2, 1, 2, 1, 2, 1, 2};
        for (int i = 0; i < 8; i++) {
          REQUIRE(result.chunkyPixels[i] == expected[i]);
        }
      }

      freeBitmap(result);
    }
  }

  GIVEN("A bitmap where the initial pointer bit updates the mask") {
    auto data =
        buildPackedBitmap(1, 1, 1, 1, {0x00, 0xBB}, {0x00, 0x80}, {0x80});

    WHEN("Decompressing") {
      auto result = decompress(data);

      THEN("The updated mask allows a fresh value read") {
        REQUIRE(result.bitplaneData[0][0] == 0xBB);
      }

      freeBitmap(result);
    }
  }

  GIVEN("A SPACK-format input with screen header and palette") {
    std::vector<uint8_t> spackHeader(90, 0);
    spackHeader[0] = 0x12;
    spackHeader[1] = 0x03;
    spackHeader[2] = 0x19;
    spackHeader[3] = 0x90;
    spackHeader[5] = 0x08;
    spackHeader[7] = 0x01;
    spackHeader[23] = 0x02;
    spackHeader[25] = 0x01;
    spackHeader[28] = 0x0F;
    spackHeader[29] = 0xFF;

    auto bitmapData = buildPackedBitmap(1, 1, 1, 1, {0x42}, {0x00}, {0x00});

    std::vector<uint8_t> data;
    data.insert(data.end(), spackHeader.begin(), spackHeader.end());
    data.insert(data.end(), bitmapData.begin(), bitmapData.end());

    WHEN("Decompressing") {
      auto result = decompress(data);

      THEN("Palette comes from the SPACK header, not the default ramp") {
        REQUIRE(result.palette[0] == 0x000);
        REQUIRE(result.palette[1] == 0xFFF);
      }

      THEN("Bitmap data is decompressed correctly") {
        REQUIRE(result.width == 8);
        REQUIRE(result.height == 1);
        REQUIRE(result.bitplaneData[0][0] == 0x42);
      }

      freeBitmap(result);
    }
  }

  GIVEN("A bare bitmap (no SPACK header)") {
    auto data = buildPackedBitmap(1, 1, 1, 1, {0x42}, {0x00}, {0x00});

    WHEN("Decompressing") {
      auto result = decompress(data);

      THEN("Default palette is a generated ramp") {
        REQUIRE(result.palette[0] == 0x000);
        REQUIRE(result.palette[1] == 0x111);
        REQUIRE(result.palette[15] == 0xFFF);
      }

      freeBitmap(result);
    }
  }

  GIVEN("A multi-cell bitmap with a mix of fresh and repeated values") {
    auto data =
        buildPackedBitmap(1, 1, 4, 1, {0x00, 0xAA, 0xBB}, {0x90}, {0x00});

    WHEN("Decompressing") {
      auto result = decompress(data);

      THEN("Fresh and repeated values are placed correctly") {
        REQUIRE(result.bitplaneData[0][0] == 0xAA);
        REQUIRE(result.bitplaneData[0][1] == 0xAA);
        REQUIRE(result.bitplaneData[0][2] == 0xAA);
        REQUIRE(result.bitplaneData[0][3] == 0xBB);
      }

      freeBitmap(result);
    }
  }
}

SCENARIO("AMOS Compact decompression rejects invalid input") {
  GIVEN("Input smaller than 4 bytes") {
    std::vector<uint8_t> data = {0x01, 0x02};

    WHEN("Attempting to decompress") {
      THEN("It throws") {
        REQUIRE_THROWS_AS(decompress(data), std::runtime_error);
      }
    }
  }

  GIVEN("Input with an unrecognized magic number") {
    std::vector<uint8_t> data(30, 0);
    data[0] = 0xDE;
    data[1] = 0xAD;
    data[2] = 0xBE;
    data[3] = 0xEF;

    WHEN("Attempting to decompress") {
      THEN("It throws") {
        REQUIRE_THROWS_AS(decompress(data), std::runtime_error);
      }
    }
  }

  GIVEN("A bitmap with numberOfBitplanes = 0") {
    auto data = buildPackedBitmap(1, 1, 1, 0, {0x00}, {0x00}, {0x00});

    WHEN("Attempting to decompress") {
      THEN("It throws") {
        REQUIRE_THROWS_AS(decompress(data), std::runtime_error);
      }
    }
  }

  GIVEN("A bitmap with numberOfBitplanes exceeding maximum") {
    auto data = buildPackedBitmap(1, 1, 1, 7, {0x00}, {0x00}, {0x00});

    WHEN("Attempting to decompress") {
      THEN("It throws") {
        REQUIRE_THROWS_AS(decompress(data), std::runtime_error);
      }
    }
  }

  GIVEN("A bitmap with gridX = 0") {
    auto data = buildPackedBitmap(0, 1, 1, 1, {0x00}, {0x00}, {0x00});

    WHEN("Attempting to decompress") {
      THEN("It throws") {
        REQUIRE_THROWS_AS(decompress(data), std::runtime_error);
      }
    }
  }

  GIVEN("A bitmap with gridY = 0") {
    auto data = buildPackedBitmap(1, 0, 1, 1, {0x00}, {0x00}, {0x00});

    WHEN("Attempting to decompress") {
      THEN("It throws") {
        REQUIRE_THROWS_AS(decompress(data), std::runtime_error);
      }
    }
  }

  GIVEN("A bitmap with tileHeight = 0") {
    auto data = buildPackedBitmap(1, 1, 0, 1, {0x00}, {0x00}, {0x00});

    WHEN("Attempting to decompress") {
      THEN("It throws") {
        REQUIRE_THROWS_AS(decompress(data), std::runtime_error);
      }
    }
  }

  GIVEN("A bitmap with bitstream pointer beyond data") {
    auto data = buildPackedBitmap(1, 1, 1, 1, {0x42}, {0x00}, {0x00});
    data[20] = 0x00;
    data[21] = 0x00;
    data[22] = 0xFF;
    data[23] = 0xFF;

    WHEN("Attempting to decompress") {
      THEN("It throws") {
        REQUIRE_THROWS_AS(decompress(data), std::runtime_error);
      }
    }
  }

  GIVEN("A SPACK header without enough data for the bitmap header") {
    std::vector<uint8_t> data(100, 0);
    data[0] = 0x12;
    data[1] = 0x03;
    data[2] = 0x19;
    data[3] = 0x90;

    WHEN("Attempting to decompress") {
      THEN("It throws") {
        REQUIRE_THROWS_AS(decompress(data), std::runtime_error);
      }
    }
  }
}
