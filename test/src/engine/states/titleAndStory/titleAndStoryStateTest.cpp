#include "../../../../../src/engine/states/titleAndStory/TitleAndStoryState.h"

#include "../../../../../src/engine/AmigaDisplay.h"
#include "../../../../../src/engine/assets/Assets.h"
#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::titleAndStory;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::systems::audio;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr int FADE_IN_FRAMES =
    2 * SCREEN_OPEN_VBLS + 5 + 15 * 5 + SCREEN_CLOSE_VBLS;
constexpr int FADE_OUT_FRAMES = 45 + SCREEN_CLOSE_VBLS;
constexpr int TITLE_FRAMES = FADE_IN_FRAMES + 300 + FADE_OUT_FRAMES;
constexpr int VERSION12_TITLE_FRAMES = FADE_IN_FRAMES + 100 + FADE_OUT_FRAMES;
constexpr int STORY_OPEN_FRAMES = 2 * SCREEN_OPEN_VBLS;

constexpr auto INTRO = "assets/intro.json";
constexpr auto PAGES = R"({"pages": [
  {"beat": 0, "lines": [{"y": 16, "text": "A"}]},
  {"beat": 0, "lines": [{"y": 16, "text": "B"}]},
  {"beat": 0, "lines": [{"y": 16, "text": "C"}]},
  {"beat": 0, "lines": [{"y": 16, "text": "DE"}]}]})";
constexpr int IMAGE_OFFSET = 5;

std::string glyphPath(char letter) {
  return assets::imagePath("s50", letter + IMAGE_OFFSET);
}

FakeFiles introFiles() {
  FakeFiles files;
  const std::string pages = PAGES;
  files.contents[INTRO] = std::vector<uint8_t>(pages.begin(), pages.end());
  for (const char letter : {'A', 'B', 'C', 'D', 'E'}) {
    files.bitmaps[glyphPath(letter)] = {};
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

struct Title {
  explicit Title(GameVersion version) {
    state.emplace(monitor, speaker, controller, files, version);
  }

  Title(GameVersion version, FakeFiles introFiles)
      : files(std::move(introFiles)) {
    state.emplace(monitor, speaker, controller, files, version);
  }

  FakeMonitor monitor;
  FakeSpeaker speaker;
  ControllerSystem controller;
  FakeFiles files;
  std::optional<TitleAndStoryState> state;
};

std::vector<int> fadeOut() {
  std::vector<int> volumes;
  for (int volume = 63; volume >= 0; --volume) {
    volumes.push_back(volume);
  }
  return volumes;
}

} // namespace

SCENARIO("The title picture is read on entry") {
  GIVEN("Version 1.0") {
    Title title(GameVersion::V10);

    THEN("It is 03BA") {
      REQUIRE(title.files.loaded ==
              std::vector<std::string>{"assets/03BA.bmp"});
    }
  }

  GIVEN("Version 1.2") {
    Title title(GameVersion::V12);

    THEN("It is p54") {
      REQUIRE(title.files.loaded == std::vector<std::string>{"assets/p54.bmp"});
    }
  }
}

SCENARIO("Fire pressed during the title skips the story") {
  GIVEN("Version 1.0") {
    Title title(GameVersion::V10);
    latchFire(title.controller);
    const Exit exit = runToExit(*title.state, 1000);

    THEN("The code card check follows the title's FOTO") {
      REQUIRE(exit.next == EngineStateId::ProtectionCheck);
      REQUIRE(exit.frames == TITLE_FRAMES);
      REQUIRE(title.speaker.volumes.empty());
    }
  }

  GIVEN("Version 1.2") {
    Title title(GameVersion::V12);
    latchFire(title.controller);
    const Exit exit = runToExit(*title.state, 1000);

    THEN("The music fades out and the scores follow") {
      REQUIRE(exit.next == EngineStateId::HighScore);
      REQUIRE(exit.frames == VERSION12_TITLE_FRAMES + 64);
      REQUIRE(title.speaker.volumes == fadeOut());
      REQUIRE(title.speaker.musicStops == 1);
    }
  }
}

SCENARIO("Left alone, the title opens the story") {
  GIVEN("Version 1.0") {
    Title title(GameVersion::V10);
    run(*title.state, TITLE_FRAMES + STORY_OPEN_FRAMES);

    THEN("The screens open before any story picture is read") {
      REQUIRE(title.files.loaded ==
              std::vector<std::string>{"assets/03BA.bmp"});
    }

    WHEN("The story starts") {
      run(*title.state, 1);

      THEN("Its first frame is read from 03BE") {
        REQUIRE(title.files.wasLoaded(assets::partPath("03BE", 0)));
      }
    }
  }
}

SCENARIO("Version 1.2 reads the next page's letters while a page holds lit") {
  GIVEN("Four pages of credits, the first two shown before the knee") {
    Title title(GameVersion::V12, introFiles());

    WHEN("The title is still up") {
      run(*title.state, VERSION12_TITLE_FRAMES);

      THEN("No letter has been read") {
        REQUIRE(title.files.loaded ==
                std::vector<std::string>{"assets/p54.bmp", INTRO});
      }
    }

    WHEN("The third page is fading in") {
      run(*title.state, VERSION12_TITLE_FRAMES + 4);

      THEN("Only its own letter has been read") {
        REQUIRE(title.files.wasLoaded(glyphPath('C')));
        REQUIRE_FALSE(title.files.wasLoaded(glyphPath('D')));
        REQUIRE_FALSE(title.files.wasLoaded(glyphPath('E')));
      }
    }

    WHEN("The third page has been fully lit for a few frames") {
      run(*title.state, VERSION12_TITLE_FRAMES + 40);

      THEN("The fourth page's letters are read ahead") {
        REQUIRE(title.files.wasLoaded(glyphPath('D')));
        REQUIRE(title.files.wasLoaded(glyphPath('E')));
      }

      THEN("The pages shown before the knee are not read") {
        REQUIRE_FALSE(title.files.wasLoaded(glyphPath('A')));
        REQUIRE_FALSE(title.files.wasLoaded(glyphPath('B')));
      }
    }
  }
}
