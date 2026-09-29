#include "../../../lib/converter/headers/headers.h"

#include <catch2/catch_all.hpp>

#include <vector>

using namespace openfranko::lib::converter::headers;

namespace {

void putBigEndian16(std::vector<uint8_t> &buf, size_t off, uint16_t v) {
  buf[off] = static_cast<uint8_t>(v >> 8);
  buf[off + 1] = static_cast<uint8_t>(v);
}

void putBigEndian32(std::vector<uint8_t> &buf, size_t off, uint32_t v) {
  buf[off] = static_cast<uint8_t>(v >> 24);
  buf[off + 1] = static_cast<uint8_t>(v >> 16);
  buf[off + 2] = static_cast<uint8_t>(v >> 8);
  buf[off + 3] = static_cast<uint8_t>(v);
}

void putBigEndian16Signed(std::vector<uint8_t> &buf, size_t off, int16_t v) {
  putBigEndian16(buf, off, static_cast<uint16_t>(v));
}

} // namespace

SCENARIO("parseSpackHeader reads all fields from big-endian data") {
  GIVEN("A buffer with known SPACK header values") {
    std::vector<uint8_t> data(90, 0);
    putBigEndian16(data, 4, 320);
    putBigEndian16(data, 6, 256);
    putBigEndian16(data, 8, 10);
    putBigEndian16(data, 10, 20);
    putBigEndian16(data, 12, 300);
    putBigEndian16(data, 14, 200);
    putBigEndian16(data, 16, 0);
    putBigEndian16(data, 18, 0);
    putBigEndian16(data, 20, 0x00);
    putBigEndian16(data, 22, 16);
    putBigEndian16(data, 24, 4);
    putBigEndian16(data, 26, 0x0F00);
    putBigEndian16(data, 28, 0x00F0);

    WHEN("parseSpackHeader is called") {
      auto header = parseSpackHeader(data);

      THEN("Screen dimensions are correct") {
        REQUIRE(header.screenWidth == 320);
        REQUIRE(header.screenHeight == 256);
      }

      THEN("Window position and size are correct") {
        REQUIRE(header.windowX == 10);
        REQUIRE(header.windowY == 20);
        REQUIRE(header.windowWidth == 300);
        REQUIRE(header.windowHeight == 200);
      }

      THEN("Color info is correct") {
        REQUIRE(header.numberOfColors == 16);
        REQUIRE(header.numberOfBitplanes == 4);
      }

      THEN("Palette values are read") {
        REQUIRE(header.amigaPalette[0] == 0x0F00);
        REQUIRE(header.amigaPalette[1] == 0x00F0);
      }
    }
  }
}

SCENARIO("parseBitmapHeader reads all fields from big-endian data") {
  GIVEN("A buffer with known bitmap header values") {
    std::vector<uint8_t> data(24, 0);
    putBigEndian16Signed(data, 4, -5);
    putBigEndian16Signed(data, 6, 10);
    putBigEndian16(data, 8, 16);
    putBigEndian16(data, 10, 16);
    putBigEndian16(data, 12, 32);
    putBigEndian16(data, 14, 5);
    putBigEndian32(data, 16, 0x1000);
    putBigEndian32(data, 20, 0x2000);

    WHEN("parseBitmapHeader is called") {
      auto header = parseBitmapHeader(data);

      THEN("Signed offsets are correct") {
        REQUIRE(header.xOffset == -5);
        REQUIRE(header.yOffset == 10);
      }

      THEN("Grid dimensions are correct") {
        REQUIRE(header.gridX == 16);
        REQUIRE(header.gridY == 16);
      }

      THEN("Tile height is correct") { REQUIRE(header.tileHeight == 32); }

      THEN("Bitplane count is correct") {
        REQUIRE(header.numberOfBitplanes == 5);
      }

      THEN("Data offsets are correct") {
        REQUIRE(header.offsetToByteTable2 == 0x1000);
        REQUIRE(header.offsetToPointerBitstream == 0x2000);
      }
    }
  }
}

SCENARIO("parseSpackHeader reads a full 90-byte SPACK header") {
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
      auto header = parseSpackHeader(data);

      THEN("All fields are correct") {
        REQUIRE(header.screenWidth == 320);
        REQUIRE(header.screenHeight == 200);
        REQUIRE(header.windowX == 16);
        REQUIRE(header.windowY == 32);
        REQUIRE(header.windowWidth == 304);
        REQUIRE(header.windowHeight == 184);
        REQUIRE(header.viewX == 5);
        REQUIRE(header.viewY == 10);
        REQUIRE(header.displayModeFlags == 0x8000);
        REQUIRE(header.numberOfColors == 16);
        REQUIRE(header.numberOfBitplanes == 4);
        REQUIRE(header.amigaPalette[0] == 0x000);
        REQUIRE(header.amigaPalette[1] == 0xF00);
      }
    }
  }
}

SCENARIO("parseBitmapHeader reads a full 24-byte bitmap header") {
  GIVEN("A 24-byte bitmap header with known values") {
    std::vector<uint8_t> data = {
        0x06, 0x07, 0x19, 0x63, 0xFF, 0xFE, 0x00, 0x03, 0x00, 0x28, 0x00, 0x0A,
        0x00, 0x10, 0x00, 0x04, 0x00, 0x00, 0x12, 0x34, 0x00, 0x00, 0x56, 0x78,
    };

    WHEN("Parsing the header") {
      auto header = parseBitmapHeader(data);

      THEN("All fields are correct including signed offsets") {
        REQUIRE(header.xOffset == -2);
        REQUIRE(header.yOffset == 3);
        REQUIRE(header.gridX == 40);
        REQUIRE(header.gridY == 10);
        REQUIRE(header.tileHeight == 16);
        REQUIRE(header.numberOfBitplanes == 4);
        REQUIRE(header.offsetToByteTable2 == 0x1234);
        REQUIRE(header.offsetToPointerBitstream == 0x5678);
      }
    }
  }
}
