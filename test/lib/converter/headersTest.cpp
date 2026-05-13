#include "../../../lib/converter/shared/headers.h"
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

      THEN("tile height is correct") {
        REQUIRE(hdr.tileHeight == 32);
      }

      THEN("bitplane count is correct") {
        REQUIRE(hdr.numberOfBitplanes == 5);
      }

      THEN("data offsets are correct") {
        REQUIRE(hdr.offsetToByteTable2 == 0x1000);
        REQUIRE(hdr.offsetToPointerBitstream == 0x2000);
      }
    }
  }
}
