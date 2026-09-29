#include "../../../lib/converter/headers/headers.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::lib::converter::headers;

namespace {

void putBE16(std::vector<uint8_t> &buf, size_t off, uint16_t v) {
  buf[off] = static_cast<uint8_t>(v >> 8);
  buf[off + 1] = static_cast<uint8_t>(v);
}

void putBE32(std::vector<uint8_t> &buf, size_t off, uint32_t v) {
  buf[off] = static_cast<uint8_t>(v >> 24);
  buf[off + 1] = static_cast<uint8_t>(v >> 16);
  buf[off + 2] = static_cast<uint8_t>(v >> 8);
  buf[off + 3] = static_cast<uint8_t>(v);
}

void putBE16S(std::vector<uint8_t> &buf, size_t off, int16_t v) {
  putBE16(buf, off, static_cast<uint16_t>(v));
}

} // namespace

SCENARIO("parseSPACKHeader reads all fields from big-endian data") {
  GIVEN("A buffer with known SPACK header values") {
    std::vector<uint8_t> data(90, 0);
    putBE16(data, 4, 320);
    putBE16(data, 6, 256);
    putBE16(data, 8, 10);
    putBE16(data, 10, 20);
    putBE16(data, 12, 300);
    putBE16(data, 14, 200);
    putBE16(data, 16, 0);
    putBE16(data, 18, 0);
    putBE16(data, 20, 0x00);
    putBE16(data, 22, 16);
    putBE16(data, 24, 4);
    putBE16(data, 26, 0x0F00);
    putBE16(data, 28, 0x00F0);

    WHEN("parseSPACKHeader is called") {
      auto hdr = parseSPACKHeader(data);

      THEN("screen dimensions are correct") {
        REQUIRE(hdr.screenWidth == 320);
        REQUIRE(hdr.screenHeight == 256);
      }

      THEN("window position and size are correct") {
        REQUIRE(hdr.windowX == 10);
        REQUIRE(hdr.windowY == 20);
        REQUIRE(hdr.windowWidth == 300);
        REQUIRE(hdr.windowHeight == 200);
      }

      THEN("color info is correct") {
        REQUIRE(hdr.numberOfColors == 16);
        REQUIRE(hdr.numberOfBitplanes == 4);
      }

      THEN("palette values are read") {
        REQUIRE(hdr.amigaPalette[0] == 0x0F00);
        REQUIRE(hdr.amigaPalette[1] == 0x00F0);
      }
    }
  }
}

SCENARIO("parseBitmapHeader reads all fields from big-endian data") {
  GIVEN("A buffer with known bitmap header values") {
    std::vector<uint8_t> data(24, 0);
    putBE16S(data, 4, -5);
    putBE16S(data, 6, 10);
    putBE16(data, 8, 16);
    putBE16(data, 10, 16);
    putBE16(data, 12, 32);
    putBE16(data, 14, 5);
    putBE32(data, 16, 0x1000);
    putBE32(data, 20, 0x2000);

    WHEN("parseBitmapHeader is called") {
      auto hdr = parseBitmapHeader(data);

      THEN("signed offsets are correct") {
        REQUIRE(hdr.xOffset == -5);
        REQUIRE(hdr.yOffset == 10);
      }

      THEN("grid dimensions are correct") {
        REQUIRE(hdr.gridX == 16);
        REQUIRE(hdr.gridY == 16);
      }

      THEN("tile height is correct") { REQUIRE(hdr.tileHeight == 32); }

      THEN("bitplane count is correct") { REQUIRE(hdr.numberOfBitplanes == 5); }

      THEN("data offsets are correct") {
        REQUIRE(hdr.offsetToByteTable2 == 0x1000);
        REQUIRE(hdr.offsetToPointerBitstream == 0x2000);
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
