#include "../../../../../src/engine/states/menu/MenuPrefetch.h"

#include "../../../../../src/engine/assets/Assets.h"
#include "../../../../../src/engine/effects/sequences/MenuSequence.h"
#include "../../../../../src/engine/states/menu/MenuState.h"
#include "../../../../../src/systems/audio/AudioSystem.h"
#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::effects::sequences;
using namespace openfranko::src::engine::states::menu;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::src::systems::audio;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::systems::audio;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr int FRAME_LIMIT = 200;
constexpr int OPENING_FRAMES = 56;
constexpr int ATTRACT_SCREEN_WIDTH = 320;

std::vector<std::string> joined(std::vector<std::string> first,
                                const std::vector<std::string> &second) {
  first.insert(first.end(), second.begin(), second.end());
  return first;
}

struct Prefetch {
  explicit Prefetch(bool withTune = true) {
    auto fake = std::make_unique<FakeFiles>();
    if (withTune) {
      fake->contents[MenuState::tunePath(GameVersion::V12)] = {'S', 'C', 'R',
                                                               'M'};
    }
    inner = fake.get();
    files = std::make_unique<assets::PrefetchingFiles>(std::move(fake));
    prefetch.emplace(*files, audio);
  }

  int framesUntilIdle() {
    for (int frame = 1; frame <= FRAME_LIMIT; ++frame) {
      const std::size_t read = inner->loaded.size();
      prefetch->step();
      if (inner->loaded.size() == read) {
        return frame;
      }
    }
    return FRAME_LIMIT;
  }

  FakeFiles *inner = nullptr;
  std::unique_ptr<assets::PrefetchingFiles> files;
  AudioSystem audio{[](const std::string &) { return std::vector<uint8_t>{}; }};
  std::optional<MenuPrefetch> prefetch;
};

} // namespace

SCENARIO("The menu's files are named for each version") {
  GIVEN("Version 1.0") {
    const auto menu = MenuState::menuPaths(GameVersion::V10);
    const auto attract = MenuState::attractPaths(GameVersion::V10);

    THEN("The menu needs its backdrop and 28 bobs, the attract two pictures "
         "and 41 letters") {
      REQUIRE(menu.size() == 29);
      REQUIRE(menu.front() == "assets/03B8.bmp");
      REQUIRE(menu.back() == assets::imagePath("0034", 27));
      REQUIRE(attract.size() == 43);
      REQUIRE(attract[0] == "assets/03BA.bmp");
      REQUIRE(attract[1] == "assets/03B9.bmp");
      REQUIRE(attract.back() == assets::imagePath("0035", 40));
      REQUIRE(MenuState::tunePath(GameVersion::V10) == "assets/0261.s3m");
    }
  }

  GIVEN("Version 1.2") {
    const auto menu = MenuState::menuPaths(GameVersion::V12);
    const auto attract = MenuState::attractPaths(GameVersion::V12);

    THEN("The menu needs its backdrop and 25 bobs, the attract two pictures "
         "and 41 letters") {
      REQUIRE(menu.size() == 26);
      REQUIRE(menu.front() == "assets/p52.bmp");
      REQUIRE(menu.back() == assets::imagePath("s52", 24));
      REQUIRE(attract.size() == 43);
      REQUIRE(attract[0] == "assets/p54.bmp");
      REQUIRE(attract[1] == "assets/p53.bmp");
      REQUIRE(attract.back() == assets::imagePath("s53", 40));
      REQUIRE(MenuState::tunePath(GameVersion::V12) == "assets/m9.s3m");
    }
  }
}

SCENARIO("The menu's files are read ahead a few a frame") {
  GIVEN("A prefetch started for version 1.0") {
    Prefetch prefetch;
    prefetch.prefetch->start(GameVersion::V10);

    WHEN("A frame passes") {
      prefetch.prefetch->step();

      THEN("No more than two files have been read") {
        REQUIRE(prefetch.inner->loaded.size() <= 2);
      }
    }

    WHEN("Frames pass until nothing more is read") {
      const int frames = prefetch.framesUntilIdle();

      THEN("The menu's pictures came first, then the attract's, and no tune") {
        REQUIRE(frames < FRAME_LIMIT);
        REQUIRE(prefetch.inner->loaded ==
                joined(MenuState::menuPaths(GameVersion::V10),
                       MenuState::attractPaths(GameVersion::V10)));
      }

      THEN("Further frames read nothing") {
        const std::size_t read = prefetch.inner->loaded.size();
        for (int frame = 0; frame < 10; ++frame) {
          prefetch.prefetch->step();
        }
        REQUIRE(prefetch.inner->loaded.size() == read);
      }
    }
  }

  GIVEN("A prefetch started for version 1.2") {
    Prefetch prefetch;
    prefetch.prefetch->start(GameVersion::V12);
    prefetch.framesUntilIdle();

    THEN("The menu tune is read before the pictures") {
      REQUIRE(prefetch.inner->loaded ==
              joined({"assets/m9.s3m"},
                     joined(MenuState::menuPaths(GameVersion::V12),
                            MenuState::attractPaths(GameVersion::V12))));
    }
  }

  GIVEN("A prefetch started for version 1.2 without the menu tune") {
    Prefetch prefetch(false);
    prefetch.prefetch->start(GameVersion::V12);
    prefetch.framesUntilIdle();

    THEN("The pictures are read ahead all the same") {
      REQUIRE(prefetch.inner->loaded ==
              joined({"assets/m9.s3m"},
                     joined(MenuState::menuPaths(GameVersion::V12),
                            MenuState::attractPaths(GameVersion::V12))));
    }
  }
}

SCENARIO("The prefetch can be paused, stopped and started over") {
  GIVEN("A prefetch that has run a few frames") {
    Prefetch prefetch;
    prefetch.prefetch->start(GameVersion::V10);
    for (int frame = 0; frame < 3; ++frame) {
      prefetch.prefetch->step();
    }
    const std::size_t read = prefetch.inner->loaded.size();
    const std::string first = MenuState::menuPaths(GameVersion::V10).front();

    WHEN("It is paused") {
      prefetch.prefetch->pause();
      for (int frame = 0; frame < 10; ++frame) {
        prefetch.prefetch->step();
      }

      THEN("Nothing more is read and what was read is kept") {
        REQUIRE(read > 0);
        REQUIRE(prefetch.inner->loaded.size() == read);
        prefetch.files->loadBitmap(first);
        REQUIRE(prefetch.inner->loaded.size() == read);
      }
    }

    WHEN("It is stopped") {
      prefetch.prefetch->stop();
      for (int frame = 0; frame < 10; ++frame) {
        prefetch.prefetch->step();
      }

      THEN("Nothing more is read and what was read is dropped") {
        REQUIRE(prefetch.inner->loaded.size() == read);
        prefetch.files->loadBitmap(first);
        REQUIRE(prefetch.inner->loaded.size() == read + 1);
      }
    }

    WHEN("It is started again") {
      prefetch.prefetch->start(GameVersion::V10);
      prefetch.prefetch->step();

      THEN("It reads from the first picture again") {
        REQUIRE(prefetch.inner->loaded.size() > read);
        REQUIRE(prefetch.inner->loaded[read] == first);
      }
    }
  }
}

namespace {

struct PrefetchedMenu {
  explicit PrefetchedMenu(GameVersion version) {
    prefetch.prefetch->start(version);
    prefetch.framesUntilIdle();
    read = prefetch.inner->loaded.size();
    session.version = version;
    state.emplace(monitor, speaker, controller, *prefetch.files, options,
                  session);
  }

  Prefetch prefetch;
  std::size_t read = 0;
  FakeMonitor monitor;
  FakeSpeaker speaker;
  ControllerSystem controller;
  GameOptions options;
  GameSession session;
  std::optional<MenuState> state;
};

} // namespace

SCENARIO("A menu opened after the prefetch reads no file of its own") {
  for (const GameVersion version : {GameVersion::V10, GameVersion::V12}) {
    GIVEN("Version " +
          std::string(version == GameVersion::V12 ? "1.2" : "1.0")) {
      PrefetchedMenu menu(version);

      WHEN("The menu opens and the attract starts") {
        run(*menu.state, OPENING_FRAMES + MenuSequence::ATTRACT_AFTER + 10);

        THEN("Every picture came from the prefetch") {
          REQUIRE(menu.monitor.width == ATTRACT_SCREEN_WIDTH);
          REQUIRE(menu.prefetch.inner->loaded.size() == menu.read);
        }
      }
    }
  }
}
