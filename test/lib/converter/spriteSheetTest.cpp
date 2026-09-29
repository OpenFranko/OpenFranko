#include "../../../lib/converter/spriteSheet/spriteSheet.h"

#include "../../../lib/binary/binary.h"
#include "../../../lib/converter/gameData/palettes.h"
#include "buildPackedBitmap.h"

#include <catch2/catch_all.hpp>

#include <vector>

using namespace openfranko::lib::converter::spriteSheet;
using namespace openfranko::lib::binary;
using namespace openfranko::lib::converter::gameData;
using namespace openfranko::test::lib::converter;

namespace {

std::vector<uint8_t>
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

} // namespace

SCENARIO("parseHeader reads sprite bank header and descriptors") {
  GIVEN("A header with 2 sprites") {
    std::vector<SpriteDescriptor> descs = {
        {100, 4, 32, 8, 16},
        {200, 2, 16, 4, 8},
    };
    auto data = buildBankHeader(2, 64, 32, 16, 0x1000, descs);

    WHEN("parseHeader is called") {
      auto header = parseHeader(data);

      THEN("The header fields are correct") {
        REQUIRE(header.count == 2);
        REQUIRE(header.maxWidth == 64);
        REQUIRE(header.maxHeight == 32);
        REQUIRE(header.numberOfColors == 16);
        REQUIRE(header.samBankOffset == 0x1000);
      }

      THEN("The descriptors are correct") {
        REQUIRE(header.descriptors.size() == 2);
        REQUIRE(header.descriptors[0].wordOffset == 100);
        REQUIRE(header.descriptors[0].widthWords == 4);
        REQUIRE(header.descriptors[0].height == 32);
        REQUIRE(header.descriptors[0].hotspotX == 8);
        REQUIRE(header.descriptors[0].hotspotY == 16);
        REQUIRE(header.descriptors[1].wordOffset == 200);
        REQUIRE(header.descriptors[1].widthWords == 2);
        REQUIRE(header.descriptors[1].height == 16);
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

    for (int i = 0; i < 10; ++i)
      buf.push_back(0);

    WHEN("parseHeader is called") {
      THEN("It throws due to insufficient descriptor table") {
        REQUIRE_THROWS_AS(parseHeader(buf), std::runtime_error);
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
    auto bitmap = buildPackedBitmap(1, 1, 1, 1, {0x42}, {0x00}, {0x00});
    data.insert(data.end(), bitmap.begin(), bitmap.end());
    const std::size_t noBitmapPos = 12 + 29 * 2;
    data.resize(noBitmapPos + 24, 0);
    std::vector<uint16_t> palette(palettes::LEVEL.begin(),
                                  palettes::LEVEL.end());

    WHEN("convertToIndividual is called") {
      auto sprites = convertToIndividual(data, palette);
      REQUIRE(sprites.size() == 3);

      THEN("The valid sprite is converted to a BMP") {
        REQUIRE(sprites[0].error.empty());
        REQUIRE(sprites[0].data.size() > 2);
        REQUIRE(sprites[0].data[0] == 'B');
        REQUIRE(sprites[0].data[1] == 'M');
      }

      THEN("A sprite pointing at non-bitmap data says so") {
        REQUIRE(sprites[1].data.empty());
        REQUIRE(sprites[1].error == "Invalid bitmap magic number");
      }

      THEN("A sprite pointing past the end says so") {
        REQUIRE(sprites[2].data.empty());
        REQUIRE(sprites[2].error == "Data too small for bitmap header");
      }
    }

    WHEN("convertToSheet is called") {
      auto sheet = convertToSheet(data, palette);
      REQUIRE(sheet.spriteErrors.size() == 3);

      THEN("The sheet is still written as a BMP") {
        REQUIRE(sheet.data.size() > 2);
        REQUIRE(sheet.data[0] == 'B');
        REQUIRE(sheet.data[1] == 'M');
      }

      THEN("Each skipped sprite has its reason") {
        REQUIRE(sheet.spriteErrors[0].empty());
        REQUIRE(sheet.spriteErrors[1] == "Invalid bitmap magic number");
        REQUIRE(sheet.spriteErrors[2] == "Data too small for bitmap header");
      }
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
      return std::vector<uint8_t>(sprite.data.begin() + 58,
                                  sprite.data.begin() + 66);
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

    WHEN("They come from s56 or s50, the 1.2 banks with the same font") {
      auto intro = sprites;
      applySpritePaletteFixes("s56", sprites);
      applySpritePaletteFixes("s50", intro);

      THEN("Font sprite 43 gets white and grey in both") {
        REQUIRE(entries1And2(sprites[43]) == fixedEntries);
        REQUIRE(entries1And2(intro[43]) == fixedEntries);
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

      THEN("It stays empty") { REQUIRE(sprites[43].data.empty()); }
    }
  }
}

SCENARIO("applyScreenPalette colours s50's logo reflection like the logo") {
  GIVEN("Eleven converted sprites and a 16-color packed screen") {
    std::vector<ConvertedSprite> sprites(
        11, ConvertedSprite{std::vector<uint8_t>(1078, 0), {}});
    std::vector<uint8_t> screen;
    pushBigEndian32(screen, 0x12031990);
    for (uint16_t value : {320, 256, 0, 0, 320, 256, 0, 0, 0, 16, 4}) {
      pushBigEndian16(screen, value);
    }
    for (uint16_t color = 0; color < 32; ++color) {
      pushBigEndian16(screen, color == 14   ? 0x035
                              : color == 16 ? 0xFFF
                                            : 0x000);
    }
    auto entry = [](const ConvertedSprite &sprite, int index) {
      return std::vector<uint8_t>(sprite.data.begin() + 54 + index * 4,
                                  sprite.data.begin() + 58 + index * 4);
    };
    const std::vector<uint8_t> unchanged(4, 0);

    WHEN("They come from s50") {
      applyScreenPalette("s50", screen, sprites);

      THEN("Sprites 4 to 9 take the screen's colours") {
        const std::vector<uint8_t> navy = {0x55, 0x33, 0x00, 0x00};
        REQUIRE(entry(sprites[4], 14) == navy);
        REQUIRE(entry(sprites[9], 14) == navy);
      }

      THEN("Colours past the screen's 16 and the other sprites stay") {
        REQUIRE(entry(sprites[4], 16) == unchanged);
        REQUIRE(entry(sprites[3], 14) == unchanged);
        REQUIRE(entry(sprites[10], 14) == unchanged);
      }
    }

    WHEN("They come from another bank") {
      applyScreenPalette("0038", screen, sprites);

      THEN("Nothing changes") { REQUIRE(entry(sprites[4], 14) == unchanged); }
    }

    WHEN("The screen is not a packed screen") {
      THEN("It throws a runtime_error") {
        REQUIRE_THROWS_AS(
            applyScreenPalette("s50", std::vector<uint8_t>(90, 0), sprites),
            std::runtime_error);
      }
    }
  }

  GIVEN("The banks that are shown on another file's screen") {
    THEN("Only s50 names one, the logo p50") {
      REQUIRE(paletteScreen("s50") == "p50");
      REQUIRE(paletteScreen("0038").empty());
    }
  }
}
