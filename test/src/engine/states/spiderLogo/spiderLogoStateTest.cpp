#include "../../../../../src/engine/states/spiderLogo/SpiderLogoState.h"

#include "../../../../../src/engine/assets/Assets.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::spiderLogo;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::systems::audio;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr auto STEP = "spiderStep";
constexpr auto JINGLE = "spiderJingle";
constexpr auto STEP_PATH = "assets/s50/s50_sam1_8363Hz.wav";
constexpr auto JINGLE_PATH = "assets/s50/s50_sam2_8363Hz.wav";
constexpr int STEP_VOICES = 0x1;
constexpr int JINGLE_VOICES = 0x3;
constexpr int IMAGES = 10;
constexpr int WALK_SETUP = 13;
constexpr int WALK_TEMPO = 14;

FakeFiles spiderFiles() {
  FakeFiles files;
  files.contents[STEP_PATH] = {};
  files.contents[JINGLE_PATH] = {};
  return files;
}

struct Spider {
  FakeMonitor monitor;
  FakeSpeaker speaker;
  FakeFiles files = spiderFiles();
  std::optional<SpiderLogoState> state{std::in_place, monitor, speaker, files};

  int played(const std::string &name, int voices) const {
    return static_cast<int>(std::count(speaker.plays.begin(),
                                       speaker.plays.end(),
                                       FakeSpeaker::Play{name, voices}));
  }
};

} // namespace

SCENARIO("The spider's pictures, samples and tune are loaded up front") {
  GIVEN("The spider logo state") {
    Spider spider;

    THEN("The logo comes first, then the ten walking images") {
      std::vector<std::string> expected{"assets/p50.bmp"};
      for (int image = 0; image < IMAGES; ++image) {
        expected.push_back(assets::imagePath("s50", image));
      }
      REQUIRE(spider.files.loaded == expected);
    }

    THEN("The step and the jingle are found whatever their rates") {
      REQUIRE(spider.speaker.samples ==
              std::map<std::string, std::string>{{STEP, STEP_PATH},
                                                 {JINGLE, JINGLE_PATH}});
      REQUIRE(spider.speaker.music == "assets/m11.s3m");
    }

    WHEN("The state is left") {
      spider.state.reset();

      THEN("Both samples are cleared") {
        REQUIRE(spider.speaker.samples.empty());
      }
    }
  }
}

SCENARIO("The tune starts once as the spider starts walking") {
  GIVEN("The screens still opening") {
    Spider spider;
    run(*spider.state, WALK_SETUP);

    THEN("Nothing plays yet") {
      REQUIRE(spider.speaker.musicOnceStarts == 0);
      REQUIRE(spider.speaker.tempos.empty());
    }

    WHEN("The walk is set up") {
      run(*spider.state, 1);

      THEN("The tune is played once, and slowed two frames later") {
        REQUIRE(spider.speaker.musicOnceStarts == 1);
        REQUIRE(spider.speaker.musicStarts == 0);
        run(*spider.state, 1);
        REQUIRE(spider.speaker.tempos.empty());
        run(*spider.state, 1);
        REQUIRE(spider.speaker.tempos == std::vector<int>{WALK_TEMPO});
      }
    }
  }
}

SCENARIO("The adverts follow the logo, with the tune stopped") {
  GIVEN("The spider left alone") {
    Spider spider;
    const Exit exit = runToExit(*spider.state, 3000);

    THEN("Steps were heard on the first voice and the jingle once on two") {
      REQUIRE(exit.next == EngineStateId::Adverts);
      REQUIRE(spider.played(STEP, STEP_VOICES) > 1);
      REQUIRE(spider.played(JINGLE, JINGLE_VOICES) == 1);
      REQUIRE(spider.speaker.musicStops == 1);
    }
  }
}

namespace {

constexpr int LOGO_WIDTH = 320;
constexpr int LOGO_HEIGHT = 256;
constexpr int BOB_WIDTH = 32;
constexpr int BOB_HEIGHT = 16;

openfranko::src::systems::graphics::IndexedBitmap
patterned(int width, int height, int seed, int hotX, int hotY) {
  openfranko::src::systems::graphics::IndexedBitmap bitmap;
  bitmap.width = width;
  bitmap.height = height;
  bitmap.hotspotX = hotX;
  bitmap.hotspotY = hotY;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int value = (x * 7 + y * 3 + seed) % 16;
      bitmap.pixels.push_back(
          static_cast<uint8_t>((x + 2 * y) % 5 == 0 ? 0 : value));
    }
  }
  for (int color = 0; color < 16; ++color) {
    bitmap.palette.push_back(
        static_cast<uint16_t>(0xF00 - color * 0x100 + color));
  }
  return bitmap;
}

struct PaintedSpider {
  explicit PaintedSpider(bool showsSprites) {
    files.bitmaps[assets::picturePath("p50")] =
        patterned(LOGO_WIDTH, LOGO_HEIGHT, 3, 0, 0);
    for (int index = 0; index < IMAGES; ++index) {
      files.bitmaps[assets::imagePath("s50", index)] =
          patterned(BOB_WIDTH, BOB_HEIGHT, index, 5, 4);
    }
    monitor.sprites = showsSprites;
    state.emplace(monitor, speaker, files);
  }

  FakeMonitor monitor;
  FakeSpeaker speaker;
  FakeFiles files = spiderFiles();
  std::optional<SpiderLogoState> state;
};

} // namespace

SCENARIO("The spider shown as a sprite looks the same as the spider drawn") {
  GIVEN("Two spiders on the same pictures, one on a monitor that shows "
        "sprites") {
    PaintedSpider drawn(false);
    PaintedSpider sprited(true);

    WHEN("Both walk and show the logo to the end") {
      int spriteFrames = 0;
      int redraws = 0;
      const uint8_t *lastPixels = nullptr;
      std::optional<EngineStateId> next;
      for (int frame = 0; frame < 3000 && !next; ++frame) {
        next = drawn.state->update();
        REQUIRE(sprited.state->update() == next);
        REQUIRE(sprited.monitor.frame() == drawn.monitor.frame());
        const auto &layers = sprited.monitor.shown().layers;
        if (!layers.empty() && layers.front().carriesSprites) {
          ++spriteFrames;
          if (lastPixels && layers.front().pixels != lastPixels) {
            ++redraws;
          }
          lastPixels = layers.front().pixels;
        }
      }

      THEN("The walk and the logo were each painted once under the sprite") {
        REQUIRE(next == EngineStateId::Adverts);
        REQUIRE(spriteFrames > 300);
        REQUIRE(redraws == 1);
      }
    }
  }
}
