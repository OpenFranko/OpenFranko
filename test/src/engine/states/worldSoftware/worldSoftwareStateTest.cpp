#include "../../../../../src/engine/states/worldSoftware/WorldSoftwareState.h"

#include "../../../../../src/engine/AmigaDisplay.h"
#include "../../../../../src/systems/audio/Mixer.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::worldSoftware;
using namespace openfranko::src::systems::audio;
using namespace openfranko::src::systems::graphics;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::systems::audio;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr auto PICTURE = "assets/03B6.bmp";
constexpr auto SAMPLE = "worldSoftware";
constexpr auto SAMPLE_PATH = "assets/0263/0263_sam1_13160Hz.wav";
constexpr int WIDTH = 368;
constexpr int HEIGHT = 290;
constexpr uint8_t EYES = 22;
constexpr uint16_t GREEN = 0x0F0;
constexpr uint8_t MARK = 1;
constexpr uint16_t BLUE = 0x00F;
constexpr int MARKED_ROW = 2;

constexpr int HOLD_START =
    2 * SCREEN_OPEN_VBLS + 5 + 15 * 5 + SCREEN_CLOSE_VBLS;
constexpr int FRAMES = HOLD_START + 200 + 75 + SCREEN_CLOSE_VBLS;

FakeFiles pictureFiles() {
  IndexedBitmap picture;
  picture.width = WIDTH;
  picture.height = HEIGHT;
  picture.pixels.assign(static_cast<std::size_t>(WIDTH) * HEIGHT, EYES);
  std::fill_n(picture.pixels.begin() + MARKED_ROW * WIDTH, WIDTH, MARK);
  picture.palette.assign(32, 0x000);
  picture.palette[EYES] = GREEN;
  picture.palette[MARK] = BLUE;
  FakeFiles files;
  files.bitmaps[PICTURE] = picture;
  return files;
}

struct Logo {
  FakeMonitor monitor;
  FakeSpeaker speaker;
  FakeFiles files = pictureFiles();
  std::optional<WorldSoftwareState> state{std::in_place, monitor, speaker,
                                          files};
};

} // namespace

SCENARIO("The laugh is loaded for the logo and dropped after it") {
  GIVEN("The World Software logo") {
    Logo logo;

    THEN("The picture comes from the files and the laugh is in the speaker") {
      REQUIRE(logo.files.loaded == std::vector<std::string>{PICTURE});
      REQUIRE(logo.speaker.samples ==
              std::map<std::string, std::string>{{SAMPLE, SAMPLE_PATH}});
    }

    WHEN("The state is left") {
      logo.state.reset();

      THEN("The laugh is cleared") { REQUIRE(logo.speaker.samples.empty()); }
    }
  }
}

SCENARIO("The laugh plays and the eyes flash red when the logo is held") {
  GIVEN("The logo fading in") {
    Logo logo;
    run(*logo.state, HOLD_START);

    THEN("Nothing has played while it faded in") {
      REQUIRE(logo.speaker.plays.empty());
      REQUIRE(logo.monitor.pixel(100, 100) == toArgb(GREEN));
    }

    WHEN("The hold begins") {
      run(*logo.state, 1);

      THEN("The laugh plays on every voice and the eyes turn red") {
        REQUIRE(logo.speaker.plays ==
                std::vector<FakeSpeaker::Play>{{SAMPLE, Mixer::ALL_VOICES}});
        REQUIRE(logo.monitor.pixel(100, 100) == toArgb(0xF00));
      }

      THEN("It is heard only once") {
        runToExit(*logo.state, 1000);
        REQUIRE(logo.speaker.plays.size() == 1);
      }
    }
  }
}

SCENARIO("The knee animation follows the logo") {
  GIVEN("The logo left alone") {
    Logo logo;
    const Exit exit = runToExit(*logo.state, 1000);

    THEN("It leaves once FOTO has closed the screen") {
      REQUIRE(exit.next == EngineStateId::KneeAnimation);
      REQUIRE(exit.frames == FRAMES);
    }
  }
}

SCENARIO("FOTO's AMAL shakes the logo a line up and back every frame, as the "
         "released 1.0 does for bank 4") {
  GIVEN("The logo held, with its third line marked") {
    Logo logo;
    run(*logo.state, HOLD_START + 1);
    const uint32_t raisedTop = logo.monitor.pixel(0, 0);
    const uint32_t raisedSecond = logo.monitor.pixel(0, 1);
    run(*logo.state, 1);
    const uint32_t loweredTop = logo.monitor.pixel(0, 0);
    const uint32_t loweredSecond = logo.monitor.pixel(0, 1);

    THEN("On odd frames the screen sits a line higher, so the marked line "
         "is the top row, and on even frames it is back in place") {
      REQUIRE(raisedTop == toArgb(BLUE));
      REQUIRE(raisedSecond != toArgb(BLUE));
      REQUIRE(loweredTop != toArgb(BLUE));
      REQUIRE(loweredSecond == toArgb(BLUE));
    }
  }
}
