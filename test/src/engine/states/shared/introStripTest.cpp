#include "../../../../../src/engine/states/shared/IntroStrip.h"

#include "../../../../../src/engine/assets/Assets.h"
#include "../../../../../src/engine/effects/sequences/BlyskSequence.h"
#include "../../../../../src/systems/graphics/Display.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::effects::sequences;
using namespace openfranko::src::engine::states::shared;
using namespace openfranko::src::systems::graphics;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr auto INTRO = "assets/intro.json";
constexpr int IMAGE_OFFSET = 5;
constexpr int GLYPH_WIDTH = 16;
constexpr int GLYPH_HEIGHT = 8;
constexpr int LIT_FRAMES = 40;

std::string glyphPath(char letter) {
  return assets::imagePath("s50", letter + IMAGE_OFFSET);
}

FakeFiles stripFiles(const std::string &pages, const std::string &letters) {
  FakeFiles files;
  files.contents[INTRO] = std::vector<uint8_t>(pages.begin(), pages.end());
  for (const char letter : letters) {
    IndexedBitmap glyph;
    glyph.width = GLYPH_WIDTH;
    glyph.height = GLYPH_HEIGHT;
    glyph.pixels.assign(static_cast<std::size_t>(GLYPH_WIDTH * GLYPH_HEIGHT),
                        1);
    files.bitmaps[glyphPath(letter)] = glyph;
  }
  return files;
}

std::size_t litPixels(const FakeMonitor &monitor) {
  const std::vector<uint32_t> &frame = monitor.frame();
  return static_cast<std::size_t>(
      std::count(frame.begin(), frame.end(), toArgb(BlyskSequence::INK)));
}

void showFrames(IntroStrip &strip, BlyskSequence &sequence, int frames) {
  for (int frame = 0; frame < frames; ++frame) {
    sequence.advance(false);
    strip.show(sequence);
  }
}

} // namespace

SCENARIO("Letters without a glyph are left out of the strip") {
  GIVEN("A page of two letters with a glyph for only the first") {
    FakeMonitor monitor;
    FakeFiles files = stripFiles(
        R"({"pages": [{"beat": 0, "lines": [{"y": 16, "text": "AZ"}]}]})", "A");
    IntroStrip strip(monitor, files);
    BlyskSequence sequence(0, strip.pages());

    WHEN("The page is lit") {
      showFrames(strip, sequence, LIT_FRAMES);

      THEN("Only the first letter is pasted and the missing glyph is not "
           "read") {
        REQUIRE(litPixels(monitor) ==
                static_cast<std::size_t>(GLYPH_WIDTH * GLYPH_HEIGHT));
        REQUIRE(files.loaded ==
                std::vector<std::string>{INTRO, glyphPath('A')});
      }
    }
  }
}

SCENARIO("Pages past the end of the file leave the strip empty") {
  GIVEN("A file of one page and a sequence of two") {
    FakeMonitor monitor;
    FakeFiles files = stripFiles(
        R"({"pages": [{"beat": 0, "lines": [{"y": 16, "text": "A"}]}]})", "A");
    IntroStrip strip(monitor, files);
    BlyskSequence sequence(0, strip.pages() + 1);

    WHEN("The first page is lit") {
      showFrames(strip, sequence, LIT_FRAMES);

      THEN("Its letter is pasted and nothing is read ahead") {
        REQUIRE(litPixels(monitor) ==
                static_cast<std::size_t>(GLYPH_WIDTH * GLYPH_HEIGHT));
        REQUIRE(files.loaded ==
                std::vector<std::string>{INTRO, glyphPath('A')});
      }
    }

    WHEN("The second page is lit") {
      showFrames(strip, sequence, BlyskSequence::PAGE_FRAMES + LIT_FRAMES);

      THEN("The strip is empty") {
        REQUIRE_FALSE(sequence.isFinished());
        REQUIRE(sequence.page() == 1);
        REQUIRE(litPixels(monitor) == 0);
      }
    }
  }
}
