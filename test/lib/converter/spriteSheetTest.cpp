#include "../../../lib/converter/spriteSheet/spriteSheet.h"
#include "../../../lib/helpers/helpers.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::lib::converter::spriteSheet;
using openfranko::lib::helpers::pushBigEndian16;
using openfranko::lib::helpers::pushBigEndian32;
namespace pal = openfranko::lib::converter::spriteSheet::palettes;

static std::vector<uint8_t>
buildBankHeader(uint16_t count, uint16_t maxW, uint16_t maxH,
                uint16_t numberOfColors, uint32_t samBankOff,
                const std::vector<SpriteDescriptor> &descs) {
  std::vector<uint8_t> buf;
  pushBigEndian16(buf, count);
  pushBigEndian16(buf, maxW);
  pushBigEndian16(buf, maxH);
  pushBigEndian16(buf, numberOfColors);
  pushBigEndian32(buf, samBankOff);
  for (const auto &d : descs) {
    pushBigEndian16(buf, d.wordOffset);
    pushBigEndian16(buf, d.widthWords);
    pushBigEndian16(buf, d.height);
    pushBigEndian16(buf, d.hotspotX);
    pushBigEndian16(buf, d.hotspotY);
  }
  return buf;
}

SCENARIO("parseHeader reads sprite bank header and descriptors") {
  GIVEN("A header with 2 sprites") {
    std::vector<SpriteDescriptor> descs = {
        {100, 4, 32, 8, 16},
        {200, 2, 16, 4, 8},
    };
    auto data = buildBankHeader(2, 64, 32, 16, 0x1000, descs);

    WHEN("parseHeader is called") {
      auto hdr = parseHeader(data);

      THEN("The header fields are correct") {
        REQUIRE(hdr.count == 2);
        REQUIRE(hdr.maxWidth == 64);
        REQUIRE(hdr.maxHeight == 32);
        REQUIRE(hdr.numColors == 16);
        REQUIRE(hdr.samBankOffset == 0x1000);
      }

      THEN("The descriptors are correct") {
        REQUIRE(hdr.descriptors.size() == 2);
        REQUIRE(hdr.descriptors[0].wordOffset == 100);
        REQUIRE(hdr.descriptors[0].widthWords == 4);
        REQUIRE(hdr.descriptors[0].height == 32);
        REQUIRE(hdr.descriptors[0].hotspotX == 8);
        REQUIRE(hdr.descriptors[0].hotspotY == 16);
        REQUIRE(hdr.descriptors[1].wordOffset == 200);
        REQUIRE(hdr.descriptors[1].widthWords == 2);
        REQUIRE(hdr.descriptors[1].height == 16);
      }
    }
  }

  GIVEN("A buffer too small for the bank header") {
    std::vector<uint8_t> data(10, 0);

    WHEN("parseHeader is called") {
      THEN("It throws") {
        REQUIRE_THROWS_AS(parseHeader(data), std::runtime_error);
      }
    }
  }

  GIVEN("A header with count=0") {
    std::vector<SpriteDescriptor> empty;
    auto data = buildBankHeader(0, 0, 0, 0, 0, empty);

    WHEN("parseHeader is called") {
      THEN("It throws due to invalid sprite count") {
        REQUIRE_THROWS_AS(parseHeader(data), std::runtime_error);
      }
    }
  }

  GIVEN("A header claiming 5 sprites but buffer too small for descriptors") {
    std::vector<uint8_t> buf;
    pushBigEndian16(buf, 5);
    pushBigEndian16(buf, 64);
    pushBigEndian16(buf, 32);
    pushBigEndian16(buf, 16);
    pushBigEndian32(buf, 0);

    for (int i = 0; i < 10; i++)
      buf.push_back(0);

    WHEN("parseHeader is called") {
      THEN("It throws due to insufficient descriptor table") {
        REQUIRE_THROWS_AS(parseHeader(buf), std::runtime_error);
      }
    }
  }
}

SCENARIO("selectPalette returns the correct palette for known file IDs") {
  GIVEN("Known file IDs with specific palettes") {
    WHEN("selectPalette is called with '0038'") {
      auto p = selectPalette("0038");
      THEN("It returns SUNSET") {
        REQUIRE(p.size() == pal::SUNSET.size());
        REQUIRE(p[0] == pal::SUNSET[0]);
        REQUIRE(p[15] == pal::SUNSET[15]);
      }
    }

    WHEN("selectPalette is called with '0037'") {
      auto p = selectPalette("0037");
      THEN("It returns STORY") {
        REQUIRE(p.size() == pal::STORY.size());
        REQUIRE(p[0] == pal::STORY[0]);
      }
    }

    WHEN("selectPalette is called with '0034'") {
      auto p = selectPalette("0034");
      THEN("It returns MENU") {
        REQUIRE(p.size() == pal::MENU.size());
        REQUIRE(p[0] == pal::MENU[0]);
      }
    }

    WHEN("selectPalette is called with '0035'") {
      auto p = selectPalette("0035");
      THEN("It returns MENU_35") {
        REQUIRE(p.size() == pal::MENU_35.size());
        REQUIRE(p[0] == pal::MENU_35[0]);
      }
    }

    WHEN("selectPalette is called with '0036'") {
      auto p = selectPalette("0036");
      THEN("It returns CEMETERY") {
        REQUIRE(p.size() == pal::CEMETERY.size());
        REQUIRE(p[0] == pal::CEMETERY[0]);
      }
    }

    WHEN("selectPalette is called with an unknown ID") {
      auto p = selectPalette("9999");
      THEN("It returns LEVEL as default") {
        REQUIRE(p.size() == pal::LEVEL.size());
        REQUIRE(p[0] == pal::LEVEL[0]);
      }
    }
  }
}
