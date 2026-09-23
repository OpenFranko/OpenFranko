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

static std::vector<uint8_t> buildMinimalPackedBitmap() {
  const uint32_t maskBytesOffset = 25;
  const uint32_t pointerBitsOffset = 26;
  std::vector<uint8_t> buf;
  pushBigEndian32(buf, 0x06071963);
  pushBigEndian32(buf, 0);
  pushBigEndian16(buf, 1);
  pushBigEndian16(buf, 1);
  pushBigEndian16(buf, 1);
  pushBigEndian16(buf, 1);
  pushBigEndian32(buf, maskBytesOffset);
  pushBigEndian32(buf, pointerBitsOffset);
  buf.push_back(0x42);
  buf.push_back(0x00);
  buf.push_back(0x00);
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

SCENARIO("Sprite conversion says why a sprite could not be converted") {
  GIVEN("A bank with a valid sprite, one without a bitmap and one past the "
        "end") {
    std::vector<SpriteDescriptor> descs = {
        {15, 1, 1, 0, 0},
        {29, 1, 1, 0, 0},
        {1000, 1, 1, 0, 0},
    };
    auto data = buildBankHeader(3, 8, 1, 16, 0, descs);
    auto bitmap = buildMinimalPackedBitmap();
    data.insert(data.end(), bitmap.begin(), bitmap.end());
    const size_t noBitmapPos = 12 + 29 * 2;
    data.resize(noBitmapPos + 24, 0);
    std::vector<uint16_t> palette(pal::LEVEL.begin(), pal::LEVEL.end());

    WHEN("convertToIndividual is called") {
      auto sprites = convertToIndividual(data, palette);
      REQUIRE(sprites.size() == 3);

      THEN("The valid sprite is converted to a BMP") {
        REQUIRE(sprites[0].error.empty());
        REQUIRE(sprites[0].bmpData.size() > 2);
        REQUIRE(sprites[0].bmpData[0] == 'B');
        REQUIRE(sprites[0].bmpData[1] == 'M');
      }

      THEN("A sprite pointing at non-bitmap data says so") {
        REQUIRE(sprites[1].bmpData.empty());
        REQUIRE(sprites[1].error == "Invalid bitmap magic number");
      }

      THEN("A sprite pointing past the end says so") {
        REQUIRE(sprites[2].bmpData.empty());
        REQUIRE(sprites[2].error == "Data too small for bitmap header");
      }
    }

    WHEN("convertToSheet is called") {
      auto sheet = convertToSheet(data, palette);
      REQUIRE(sheet.spriteErrors.size() == 3);

      THEN("The sheet is still written as a BMP") {
        REQUIRE(sheet.bmpData.size() > 2);
        REQUIRE(sheet.bmpData[0] == 'B');
        REQUIRE(sheet.bmpData[1] == 'M');
      }

      THEN("Each skipped sprite has its reason") {
        REQUIRE(sheet.spriteErrors[0].empty());
        REQUIRE(sheet.spriteErrors[1] == "Invalid bitmap magic number");
        REQUIRE(sheet.spriteErrors[2] == "Data too small for bitmap header");
      }
    }
  }
}

SCENARIO("byName picks a palette by its command line name") {
  GIVEN("The palette names the tools accept") {
    THEN("Each returns its palette") {
      using Pal = std::vector<uint16_t>;
      REQUIRE(pal::byName("sunset") ==
              Pal(pal::SUNSET.begin(), pal::SUNSET.end()));
      REQUIRE(pal::byName("story") ==
              Pal(pal::STORY.begin(), pal::STORY.end()));
      REQUIRE(pal::byName("menu") == Pal(pal::MENU.begin(), pal::MENU.end()));
      REQUIRE(pal::byName("menu35") ==
              Pal(pal::MENU_35.begin(), pal::MENU_35.end()));
      REQUIRE(pal::byName("cemetery") ==
              Pal(pal::CEMETERY.begin(), pal::CEMETERY.end()));
    }
  }

  GIVEN("\"level\" or an unknown name") {
    THEN("It returns the level palette") {
      using Pal = std::vector<uint16_t>;
      REQUIRE(pal::byName("level") ==
              Pal(pal::LEVEL.begin(), pal::LEVEL.end()));
      REQUIRE(pal::byName("nope") == Pal(pal::LEVEL.begin(), pal::LEVEL.end()));
    }
  }
}

SCENARIO("applySpritePaletteFixes makes the sunset bank's font white") {
  GIVEN("44 converted sprites") {
    std::vector<ConvertedSprite> sprites(
        44, ConvertedSprite{std::vector<uint8_t>(1078, 0), {}});
    const std::vector<uint8_t> fixedEntries = {0xFF, 0xFF, 0xFF, 0x00,
                                               0xAA, 0xAA, 0xAA, 0x00};
    auto entries1And2 = [](const ConvertedSprite &sprite) {
      return std::vector<uint8_t>(sprite.bmpData.begin() + 58,
                                  sprite.bmpData.begin() + 66);
    };

    WHEN("They come from bank 0038") {
      applySpritePaletteFixes("0038", sprites);

      THEN("Font sprite 43 gets white and grey as colours 1 and 2") {
        REQUIRE(entries1And2(sprites[43]) == fixedEntries);
      }

      THEN("Sprite 42, before the font, is unchanged") {
        REQUIRE(entries1And2(sprites[42]) == std::vector<uint8_t>(8, 0));
      }
    }

    WHEN("They come from another bank") {
      applySpritePaletteFixes("0001", sprites);

      THEN("Nothing changes") {
        REQUIRE(entries1And2(sprites[43]) == std::vector<uint8_t>(8, 0));
      }
    }

    WHEN("A font sprite was skipped") {
      sprites[43] = {{}, "Invalid bitmap magic number"};
      applySpritePaletteFixes("0038", sprites);

      THEN("It stays empty") { REQUIRE(sprites[43].bmpData.empty()); }
    }
  }
}
