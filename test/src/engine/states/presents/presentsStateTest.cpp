#include "../../../../../src/engine/states/presents/PresentsState.h"

#include "../../../../../src/engine/assets/Assets.h"
#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::presents;
using namespace openfranko::src::systems::graphics;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::systems::audio;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr auto INTRO = "assets/intro.json";
constexpr auto PAGES = R"({"pages": [
  {"beat": 0, "lines": [{"y": 16, "text": "A"}]},
  {"beat": 0, "lines": [{"y": 16, "text": "B"}]},
  {"beat": 0, "lines": [{"y": 16, "text": "C"}]}]})";
constexpr int PAGE_FRAMES = 90;
constexpr int IMAGE_OFFSET = 5;
constexpr int STRIP_X = 160 + 148;
constexpr int STRIP_Y = 136 - 42 + 16;

std::string glyphPath(char letter) {
  return assets::imagePath("s50", letter + IMAGE_OFFSET);
}

FakeFiles introFiles() {
  FakeFiles files;
  const std::string pages = PAGES;
  files.contents[INTRO] = std::vector<uint8_t>(pages.begin(), pages.end());
  for (const char letter : {'A', 'B', 'C'}) {
    IndexedBitmap glyph;
    glyph.width = 16;
    glyph.height = 8;
    glyph.pixels.assign(static_cast<std::size_t>(16) * 8, 1);
    files.bitmaps[glyphPath(letter)] = glyph;
  }
  return files;
}

void latchFire(ControllerSystem &controller) {
  KeyEvent space;
  space.key = Key::Space;
  space.pressed = true;
  controller.receiveKey(space);
  controller.update();
}

struct Presents {
  FakeMonitor monitor;
  FakeSpeaker speaker;
  ControllerSystem controller;
  FakeFiles files = introFiles();
  PresentsState state{monitor, speaker, controller, files};
};

} // namespace

SCENARIO("The intro pages come from intro.json and the letters from s50") {
  GIVEN("Three pages of credits") {
    Presents presents;

    THEN("The pages are read on entry") {
      REQUIRE(presents.files.loaded == std::vector<std::string>{INTRO});
    }

    WHEN("The first page is lit") {
      run(presents.state, 40);

      THEN("Its letter is read and pasted in white on the strip") {
        REQUIRE(presents.files.wasLoaded(glyphPath('A')));
        REQUIRE_FALSE(presents.files.wasLoaded(glyphPath('B')));
        REQUIRE(presents.monitor.pixel(STRIP_X + 2, STRIP_Y + 2) ==
                toArgb(0xFFF));
      }

      THEN("The strip sits on a hires screen shown at double height") {
        REQUIRE(presents.monitor.width == 640);
        REQUIRE(presents.monitor.height == 256);
        REQUIRE(presents.monitor.displayHeight == 512);
      }
    }
  }
}

SCENARIO("Two pages are shown before the knee") {
  GIVEN("Three pages of credits left alone") {
    Presents presents;
    const Exit exit = runToExit(presents.state, 1000);

    THEN("The third page waits for later") {
      REQUIRE(exit.next == EngineStateId::KneeAnimation);
      REQUIRE(exit.frames == 2 * PAGE_FRAMES + 1);
      REQUIRE(presents.files.wasLoaded(glyphPath('B')));
      REQUIRE_FALSE(presents.files.wasLoaded(glyphPath('C')));
    }
  }

  GIVEN("No intro.json") {
    FakeMonitor monitor;
    FakeSpeaker speaker;
    ControllerSystem controller;
    FakeFiles files;
    PresentsState state(monitor, speaker, controller, files);

    THEN("The knee follows at once") {
      REQUIRE(state.update() == EngineStateId::KneeAnimation);
    }
  }
}

SCENARIO("Fire at the end of a page fades the music out for the scores") {
  GIVEN("Fire pressed during the first page") {
    Presents presents;
    latchFire(presents.controller);
    const Exit exit = runToExit(presents.state, 1000);

    THEN("The volume steps down a level a frame, then the music stops") {
      std::vector<int> volumes;
      for (int volume = 63; volume >= 0; --volume) {
        volumes.push_back(volume);
      }
      REQUIRE(exit.next == EngineStateId::HighScore);
      REQUIRE(exit.frames == PAGE_FRAMES + 64);
      REQUIRE(presents.speaker.volumes == volumes);
      REQUIRE(presents.speaker.musicStops == 1);
    }
  }

  GIVEN("Fire pressed before the state") {
    FakeMonitor monitor;
    FakeSpeaker speaker;
    ControllerSystem controller;
    FakeFiles files = introFiles();
    latchFire(controller);
    const PresentsState state(monitor, speaker, controller, files);

    THEN("It is forgotten") { REQUIRE_FALSE(controller.isFireLatched()); }
  }
}
