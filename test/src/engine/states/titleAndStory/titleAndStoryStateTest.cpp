#include "../../../../../src/engine/states/titleAndStory/TitleAndStoryState.h"

#include "../../../../../src/engine/AmigaDisplay.h"
#include "../../../../../src/engine/assets/Assets.h"
#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
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

namespace {

int repaints(Title &title, int frames) {
  int changes = 0;
  const uint8_t *last = nullptr;
  for (int frame = 0; frame < frames; ++frame) {
    run(*title.state, 1);
    const auto &layers = title.monitor.shown().layers;
    const uint8_t *pixels = layers.empty() ? nullptr : layers.front().pixels;
    if (frame > 0 && pixels != last) {
      ++changes;
    }
    last = pixels;
  }
  return changes;
}

} // namespace

SCENARIO("The title and story are painted only when they change") {
  GIVEN("Version 1.0") {
    Title title(GameVersion::V10);

    WHEN("The title holds lit") {
      run(*title.state, FADE_IN_FRAMES + 10);

      THEN("The picture is not painted again") {
        REQUIRE(repaints(title, 100) == 0);
      }
    }

    WHEN("The story runs for eight animation frames") {
      run(*title.state, TITLE_FRAMES + STORY_OPEN_FRAMES + 1);

      THEN("It is painted once per animation frame at most") {
        const int changes = repaints(title, 64);
        REQUIRE(changes >= 1);
        REQUIRE(changes <= 8);
      }
    }
  }

  GIVEN("Version 1.2 with four pages of credits") {
    Title title(GameVersion::V12, introFiles());

    WHEN("The third page fades in, holds and goes") {
      run(*title.state, VERSION12_TITLE_FRAMES);

      THEN("The strip is painted only when a page changes") {
        REQUIRE(repaints(title, 80) <= 3);
      }
    }
  }
}

namespace {

constexpr int STORY_LIMIT = 3000;
constexpr int STORY_IMAGES = 7;
constexpr int STORY_ANIMATION_FRAMES = 68;
constexpr int STORY_CLOSE_FRAMES = 2 * SCREEN_CLOSE_VBLS;
constexpr int FADE_FRAMES = 64;
constexpr int BLYSK_PAGE_FRAMES = 90;

bool readEvery(const Title &title, const std::string &resource, int count) {
  for (int part = 0; part < count; ++part) {
    if (!title.files.wasLoaded(assets::partPath(resource, part))) {
      return false;
    }
  }
  return true;
}

} // namespace

SCENARIO("The story shows every page's frames, picture and text") {
  GIVEN("Version 1.0 at the start of its story") {
    Title title(GameVersion::V10);
    run(*title.state, TITLE_FRAMES + STORY_OPEN_FRAMES);

    WHEN("The joystick is held to turn each page once it is read") {
      title.controller.states.right = true;
      const Exit exit = runToExit(*title.state, STORY_LIMIT);

      THEN("Every frame, picture and text was shown before the code card "
           "check") {
        REQUIRE(exit.next == EngineStateId::ProtectionCheck);
        REQUIRE(readEvery(title, "03BE", STORY_ANIMATION_FRAMES));
        REQUIRE(readEvery(title, "03BF", STORY_IMAGES));
        REQUIRE(readEvery(title, "03C0", STORY_IMAGES));
        REQUIRE(title.speaker.volumes.empty());
      }
    }

    WHEN("Fire is pressed during the first page") {
      run(*title.state, 20);
      latchFire(title.controller);
      const Exit exit = runToExit(*title.state, STORY_LIMIT);

      THEN("The story stops at the next frame and its screens close") {
        REQUIRE(exit.next == EngineStateId::ProtectionCheck);
        REQUIRE(exit.frames <= 8 + STORY_CLOSE_FRAMES);
        REQUIRE_FALSE(title.files.wasLoaded(assets::partPath("03BF", 0)));
      }
    }
  }

  GIVEN("Version 1.2 with four pages of credits") {
    Title title(GameVersion::V12, introFiles());
    run(*title.state, VERSION12_TITLE_FRAMES);

    WHEN("Fire is held through the pages and the story") {
      title.controller.states.button = true;
      const Exit exit = runToExit(*title.state, STORY_LIMIT);

      THEN("The pages and the whole 1.2 story were shown, then the music "
           "faded") {
        REQUIRE(exit.next == EngineStateId::HighScore);
        REQUIRE(exit.frames > 2 * BLYSK_PAGE_FRAMES + FADE_FRAMES);
        REQUIRE(title.files.wasLoaded(glyphPath('D')));
        REQUIRE(readEvery(title, "p58", STORY_ANIMATION_FRAMES));
        REQUIRE(readEvery(title, "p59", STORY_IMAGES));
        REQUIRE(readEvery(title, "p60", STORY_IMAGES));
        REQUIRE(title.speaker.volumes == fadeOut());
      }
    }

    WHEN("Fire is pressed while the first page is up") {
      run(*title.state, 10);
      latchFire(title.controller);
      const Exit exit = runToExit(*title.state, STORY_LIMIT);

      THEN("The rest is skipped for the music fade and the scores") {
        REQUIRE(exit.next == EngineStateId::HighScore);
        REQUIRE(exit.frames < BLYSK_PAGE_FRAMES + FADE_FRAMES);
        REQUIRE_FALSE(title.files.wasLoaded(assets::partPath("p58", 0)));
        REQUIRE(title.speaker.volumes == fadeOut());
      }
    }
  }
}
