#include "../../../lib/converter/amosCompact/amosCompact.h"
#include "../../../lib/converter/amosCompact/detail/BitReader.h"
#include "../../../lib/converter/amosCompact/detail/ByteReader.h"
#include "../../../lib/converter/amosCompact/Consts.h"
#include "../../../lib/converter/shared/headers.h"
#include "../../../lib/decompressor/helpers/helpers.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::lib::converter::amosCompact;
using namespace openfranko::lib::converter::amosCompact::detail;
using namespace openfranko::lib::converter::headers;
using openfranko::lib::decompressor::helpers::readUint32LittleEndian;

static uint32_t bmpWidth(const std::vector<uint8_t> &bmp) {
  return readUint32LittleEndian(bmp, 18);
}

static uint32_t bmpHeight(const std::vector<uint8_t> &bmp) {
  return readUint32LittleEndian(bmp, 22);
}

static uint8_t bmpPixel(const std::vector<uint8_t> &bmp, int x, int y) {
  uint32_t w = bmpWidth(bmp);
  uint32_t h = bmpHeight(bmp);
  uint32_t rowBytes = (w + 3) & ~3u;
  uint32_t pixelOff = readUint32LittleEndian(bmp, 10);
  int bmpY = static_cast<int>(h) - 1 - y;
  return bmp[pixelOff + bmpY * rowBytes + x];
}

struct BmpColor {
  uint8_t r, g, b;
};

static BmpColor bmpPalette(const std::vector<uint8_t> &bmp, int index) {
  size_t off = 54 + index * 4;
  return {bmp[off + 2], bmp[off + 1], bmp[off]};
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
      auto hdr = parseSPACKHeader(data);

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
      auto hdr = parseBitmapHeader(data);

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

SCENARIO("AMOS Compact decompression produces valid BMP output") {
  GIVEN("A minimal single-cell bitmap with mask=0 (RLE, no fresh reads)") {
    auto data = buildPackedBitmap(1, 1, 1, 1, {0x42}, {0x00}, {0x00});

    WHEN("Decompressing") {
      auto bmp = decompress(data);

      THEN("BMP has correct header") {
        REQUIRE(bmp[0] == 'B');
        REQUIRE(bmp[1] == 'M');
        REQUIRE(bmpWidth(bmp) == 8);
        REQUIRE(bmpHeight(bmp) == 1);
      }

      THEN("Pixels match plane byte 0x42 (01000010) read MSB-first") {
        std::vector<uint8_t> expected = {0, 1, 0, 0, 0, 0, 1, 0};
        for (int i = 0; i < 8; i++) {
          REQUIRE(bmpPixel(bmp, i, 0) == expected[i]);
        }
      }
    }
  }

  GIVEN("A single-cell bitmap where a mask bit triggers a fresh value read") {
    auto data = buildPackedBitmap(1, 1, 1, 1, {0x00, 0xFF}, {0x80}, {0x00});

    WHEN("Decompressing") {
      auto bmp = decompress(data);

      THEN("All pixels are 1 (plane byte 0xFF)") {
        for (int i = 0; i < 8; i++) {
          REQUIRE(bmpPixel(bmp, i, 0) == 1);
        }
      }
    }
  }

  GIVEN("A multi-cell bitmap with mask=0 repeating the same value (RLE)") {
    auto data = buildPackedBitmap(3, 1, 1, 1, {0xAA}, {0x00}, {0x00});

    WHEN("Decompressing") {
      auto bmp = decompress(data);

      THEN("All 24 pixels follow 0xAA (10101010) pattern repeated 3 times") {
        REQUIRE(bmpWidth(bmp) == 24);
        REQUIRE(bmpHeight(bmp) == 1);
        for (int i = 0; i < 24; i++) {
          uint8_t expected = (i % 2 == 0) ? 1 : 0;
          REQUIRE(bmpPixel(bmp, i, 0) == expected);
        }
      }
    }
  }

  GIVEN("A 2x2 tile grid with tileHeight=2 verifying tile traversal order") {
    auto data = buildPackedBitmap(
        2, 2, 2, 1, {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
        {0xFF}, {0x00});

    WHEN("Decompressing") {
      auto bmp = decompress(data);

      THEN("Dimensions are correct") {
        REQUIRE(bmpWidth(bmp) == 16);
        REQUIRE(bmpHeight(bmp) == 4);
      }

      THEN("Pixels confirm correct row-major tile placement") {
        REQUIRE(bmpPixel(bmp, 7, 0) == 1);
        REQUIRE(bmpPixel(bmp, 6, 0) == 0);
        REQUIRE(bmpPixel(bmp, 15, 0) == 1);
        REQUIRE(bmpPixel(bmp, 14, 0) == 1);

        REQUIRE(bmpPixel(bmp, 7, 1) == 0);
        REQUIRE(bmpPixel(bmp, 6, 1) == 1);

        REQUIRE(bmpPixel(bmp, 7, 2) == 1);
        REQUIRE(bmpPixel(bmp, 5, 2) == 1);
        REQUIRE(bmpPixel(bmp, 15, 2) == 1);

        REQUIRE(bmpPixel(bmp, 12, 3) == 1);
        REQUIRE(bmpPixel(bmp, 7, 3) == 0);
        REQUIRE(bmpPixel(bmp, 6, 3) == 1);
      }
    }
  }

  GIVEN("A two-plane bitmap combining into chunky pixels") {
    auto data =
        buildPackedBitmap(1, 1, 1, 2, {0x00, 0xAA, 0x55}, {0xFF}, {0x00});

    WHEN("Decompressing") {
      auto bmp = decompress(data);

      THEN("Pixels combine both planes into alternating 1 and 2") {
        std::vector<uint8_t> expected = {1, 2, 1, 2, 1, 2, 1, 2};
        for (int i = 0; i < 8; i++) {
          REQUIRE(bmpPixel(bmp, i, 0) == expected[i]);
        }
      }
    }
  }

  GIVEN("A bitmap where the initial pointer bit updates the mask") {
    auto data =
        buildPackedBitmap(1, 1, 1, 1, {0x00, 0xBB}, {0x00, 0x80}, {0x80});

    WHEN("Decompressing") {
      auto bmp = decompress(data);

      THEN("Pixels match 0xBB (10111011)") {
        std::vector<uint8_t> expected = {1, 0, 1, 1, 1, 0, 1, 1};
        for (int i = 0; i < 8; i++) {
          REQUIRE(bmpPixel(bmp, i, 0) == expected[i]);
        }
      }
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
      auto bmp = decompress(data);

      THEN("BMP palette comes from the SPACK header") {
        auto c0 = bmpPalette(bmp, 0);
        REQUIRE(c0.r == 0);
        REQUIRE(c0.g == 0);
        REQUIRE(c0.b == 0);

        auto c1 = bmpPalette(bmp, 1);
        REQUIRE(c1.r == 255);
        REQUIRE(c1.g == 255);
        REQUIRE(c1.b == 255);
      }

      THEN("Pixel data is correct") {
        REQUIRE(bmpPixel(bmp, 1, 0) == 1);
        REQUIRE(bmpPixel(bmp, 6, 0) == 1);
        REQUIRE(bmpPixel(bmp, 0, 0) == 0);
      }
    }
  }

  GIVEN("A bare bitmap (no SPACK header)") {
    auto data = buildPackedBitmap(1, 1, 1, 1, {0x42}, {0x00}, {0x00});

    WHEN("Decompressing") {
      auto bmp = decompress(data);

      THEN("BMP palette uses the default generated ramp") {
        auto c0 = bmpPalette(bmp, 0);
        REQUIRE(c0.r == 0);
        REQUIRE(c0.g == 0);
        REQUIRE(c0.b == 0);

        auto c1 = bmpPalette(bmp, 1);
        REQUIRE(c1.r == 17);
        REQUIRE(c1.g == 17);
        REQUIRE(c1.b == 17);

        auto c2 = bmpPalette(bmp, 2);
        REQUIRE(c2.r == 0);
        REQUIRE(c2.g == 0);
        REQUIRE(c2.b == 0);
      }
    }
  }

  GIVEN("A multi-cell bitmap with a mix of fresh and repeated values") {
    auto data =
        buildPackedBitmap(1, 1, 4, 1, {0x00, 0xAA, 0xBB}, {0x90}, {0x00});

    WHEN("Decompressing") {
      auto bmp = decompress(data);

      THEN("Repeated rows match 0xAA and fresh row matches 0xBB") {
        REQUIRE(bmpPixel(bmp, 0, 0) == 1);
        REQUIRE(bmpPixel(bmp, 1, 0) == 0);
        REQUIRE(bmpPixel(bmp, 0, 2) == 1);
        REQUIRE(bmpPixel(bmp, 1, 2) == 0);

        REQUIRE(bmpPixel(bmp, 3, 2) == 0);
        REQUIRE(bmpPixel(bmp, 3, 3) == 1);
        REQUIRE(bmpPixel(bmp, 5, 3) == 0);
        REQUIRE(bmpPixel(bmp, 7, 3) == 1);
      }
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
