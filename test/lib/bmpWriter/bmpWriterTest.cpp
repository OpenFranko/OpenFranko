#include "../../../lib/bmpWriter/bmpWriter.h"
#include "../../../lib/helpers/helpers.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::lib::bmpWriter;
using openfranko::lib::helpers::LittleEndianReader;

SCENARIO("pixelsToBmp produces a valid Windows BMP v3") {
  GIVEN("A 2x2 image with a 2-color Amiga palette") {
    uint8_t pixels[] = {0, 1, 1, 0};
    uint16_t palette[] = {0x000, 0xFFF};
    int numberOfColors = 2;

    WHEN("pixelsToBmp is called") {
      auto bmp = pixelsToBmp(2, 2, pixels, palette, numberOfColors);
      LittleEndianReader reader(bmp);

      THEN("The BMP magic bytes are 'BM'") {
        REQUIRE(bmp[0] == 'B');
        REQUIRE(bmp[1] == 'M');
      }

      THEN("The file size field matches the actual size") {
        REQUIRE(reader.readUint32(2) == bmp.size());
      }

      THEN("The pixel data offset is 14 + 40 + 256*4 = 1078") {
        REQUIRE(reader.readUint32(10) == 1078u);
      }

      THEN("The DIB header size is 40") {
        REQUIRE(reader.readUint32(14) == 40u);
      }

      THEN("Width and height are correct") {
        REQUIRE(reader.readUint32(18) == 2u);
        REQUIRE(reader.readUint32(22) == 2u);
      }

      THEN("Bits per pixel is 8") { REQUIRE(reader.readUint16(28) == 8); }

      THEN("Compression is BI_RGB (0)") {
        REQUIRE(reader.readUint32(30) == 0u);
      }

      THEN("Colors used equals numberOfColors") {
        REQUIRE(reader.readUint32(46) == 2u);
      }
    }
  }
}

SCENARIO("pixelsToBmp converts Amiga 12-bit palette to 8-bit BGRA") {
  GIVEN("A 1x1 image with palette color 0xF80") {
    uint8_t pixels[] = {0};
    uint16_t palette[] = {0xF80};

    WHEN("pixelsToBmp is called") {
      auto bmp = pixelsToBmp(1, 1, pixels, palette, 1);
      size_t palOff = 54;

      THEN("The palette entry has R=0xFF, G=0x88, B=0x00 in BGRA order") {
        REQUIRE(bmp[palOff + 0] == 0x00);
        REQUIRE(bmp[palOff + 1] == 0x88);
        REQUIRE(bmp[palOff + 2] == 0xFF);
        REQUIRE(bmp[palOff + 3] == 0x00);
      }
    }
  }
}

SCENARIO("pixelsToBmp stores rows bottom-up with 4-byte alignment") {
  GIVEN("A 3x2 image with distinct pixels per row") {
    uint8_t pixels[] = {
        10, 11, 12,
        20, 21, 22,
    };
    uint16_t palette[] = {0x000};

    WHEN("pixelsToBmp is called") {
      auto bmp = pixelsToBmp(3, 2, pixels, palette, 1);
      LittleEndianReader reader(bmp);
      uint32_t pixelOff = reader.readUint32(10);
      uint32_t rowBytes = 4;

      THEN("The bottom row of the image comes first in the file") {
        REQUIRE(bmp[pixelOff + 0] == 20);
        REQUIRE(bmp[pixelOff + 1] == 21);
        REQUIRE(bmp[pixelOff + 2] == 22);
      }

      THEN("The top row of the image comes second in the file") {
        REQUIRE(bmp[pixelOff + rowBytes + 0] == 10);
        REQUIRE(bmp[pixelOff + rowBytes + 1] == 11);
        REQUIRE(bmp[pixelOff + rowBytes + 2] == 12);
      }

      THEN("Padding byte is zero") { REQUIRE(bmp[pixelOff + 3] == 0); }
    }
  }
}

SCENARIO("pixelsToBmp pads unused palette entries with zeros") {
  GIVEN("A 1x1 image with 1 palette color") {
    uint8_t pixels[] = {0};
    uint16_t palette[] = {0x123};

    WHEN("pixelsToBmp is called") {
      auto bmp = pixelsToBmp(1, 1, pixels, palette, 1);

      THEN("Palette entry 1 is all zeros") {
        size_t entry1 = 54 + 4;
        REQUIRE(bmp[entry1 + 0] == 0);
        REQUIRE(bmp[entry1 + 1] == 0);
        REQUIRE(bmp[entry1 + 2] == 0);
        REQUIRE(bmp[entry1 + 3] == 0);
      }
    }
  }
}
