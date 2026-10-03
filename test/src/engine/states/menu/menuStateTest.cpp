#include "../../../../../src/engine/states/menu/MenuState.h"

#include "../../../../../src/engine/MenuTempo.h"
#include "../../../../../src/engine/amal/Machine.h"
#include "../../../../../src/engine/assets/Assets.h"
#include "../../../../../src/engine/effects/sequences/MenuSequence.h"
#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::amal;
using namespace openfranko::src::engine::effects::sequences;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::menu;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::systems::audio;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr int MENU_IMAGES = 28;
constexpr int LETTER_IMAGES = 41;
constexpr int OPENING_FRAMES = 54;

struct Menu {
  explicit Menu(GameVersion version = GameVersion::V10, bool ntsc = false) {
    session.version = version;
    session.nameScreenOpen = true;
    session.registers[RO] = 2;
    options.ntsc = ntsc;
    state.emplace(monitor, speaker, controller, files, options, session);
  }

  FakeMonitor monitor;
  FakeSpeaker speaker;
  ControllerSystem controller;
  FakeFiles files;
  GameOptions options;
  GameSession session;
  std::optional<MenuState> state;
};

} // namespace

SCENARIO("The menu's pictures and bobs come from the files") {
  GIVEN("Version 1.0") {
    Menu menu;
    const std::vector<std::string> &loaded = menu.files.loaded;

    THEN("The backdrop and the menu's bobs are read on entry") {
      REQUIRE(loaded.size() == 1 + MENU_IMAGES);
      REQUIRE(loaded[0] == "assets/03B8.bmp");
      REQUIRE(loaded[1] == assets::imagePath("0034", 0));
      REQUIRE(loaded.back() == assets::imagePath("0034", MENU_IMAGES - 1));
    }

    THEN("The attract pictures and letters are not read while it opens") {
      run(*menu.state, OPENING_FRAMES);
      REQUIRE(loaded.size() == 1 + MENU_IMAGES);
    }

    WHEN("The attract starts") {
      run(*menu.state, OPENING_FRAMES + MenuSequence::ATTRACT_AFTER + 10);

      THEN("The title and hiscore pictures are read, then the letters") {
        REQUIRE(loaded.size() == 3 + MENU_IMAGES + LETTER_IMAGES);
        REQUIRE(loaded[1 + MENU_IMAGES] == "assets/03BA.bmp");
        REQUIRE(loaded[2 + MENU_IMAGES] == "assets/03B9.bmp");
        REQUIRE(loaded[3 + MENU_IMAGES] == assets::imagePath("0035", 0));
        REQUIRE(loaded.back() == assets::imagePath("0035", LETTER_IMAGES - 1));
      }
    }

    THEN("The music is left to the hiscore screen before") {
      REQUIRE(menu.speaker.music.empty());
      REQUIRE(menu.speaker.musicStarts == 0);
    }

    THEN("The name screen counts as closed") {
      REQUIRE_FALSE(menu.session.nameScreenOpen);
    }
  }
}

SCENARIO("The monitor takes the standard chosen in the options") {
  GIVEN("NTSC chosen") {
    Menu menu(GameVersion::V10, true);

    THEN("The monitor is switched to NTSC") { REQUIRE(menu.monitor.ntsc); }
  }

  GIVEN("PAL chosen") {
    Menu menu(GameVersion::V10, false);

    THEN("The monitor stays PAL") { REQUIRE_FALSE(menu.monitor.ntsc); }
  }
}

SCENARIO("Version 1.2 starts the menu tune itself") {
  GIVEN("Version 1.2") {
    Menu menu(GameVersion::V12);

    THEN("m9 is loaded and played at once") {
      REQUIRE(menu.speaker.music == "assets/m9.s3m");
      REQUIRE(menu.speaker.musicStarts == 1);
    }

    THEN("Its tempo and volume are set after the wait") {
      run(*menu.state, MenuSequence::VERSION12_MUSIC_WAIT - 1);
      REQUIRE(menu.speaker.tempos.empty());
      run(*menu.state, 1);
      REQUIRE(menu.speaker.tempos == std::vector<int>{CONVERTED_MENU_TEMPO});
      REQUIRE(menu.speaker.volumes == std::vector<int>{63});
    }
  }
}

SCENARIO("Switching to NTSC leaves the menu tune at its PAL speed") {
  auto fireNtscIcon = [](Menu &menu, int opening) {
    run(*menu.state, opening);
    menu.controller.states.right = true;
    run(*menu.state, 1);
    menu.controller.states.right = false;
    run(*menu.state, 9);
    menu.controller.states.down = true;
    run(*menu.state, 1);
    menu.controller.states.down = false;
    run(*menu.state, 9);
    menu.controller.states.button = true;
    run(*menu.state, 1);
    menu.controller.states.button = false;
  };

  GIVEN("Version 1.0 with PAL chosen") {
    Menu menu(GameVersion::V10, false);

    WHEN("The NTSC icon is fired") {
      fireNtscIcon(menu, OPENING_FRAMES);

      THEN("The monitor goes NTSC and the tune's tempo stays 37") {
        REQUIRE(menu.options.ntsc);
        REQUIRE(menu.monitor.ntsc);
        REQUIRE(menu.speaker.tempoScales == std::vector<double>{1.0});
      }
    }
  }

  GIVEN("Version 1.2 with PAL chosen") {
    Menu menu(GameVersion::V12, false);

    WHEN("The NTSC icon is fired") {
      fireNtscIcon(menu, OPENING_FRAMES + MenuSequence::VERSION12_MUSIC_WAIT);

      THEN("Tempo 37 is set again rather than 32") {
        REQUIRE(menu.options.ntsc);
        REQUIRE(menu.speaker.tempos ==
                std::vector<int>{CONVERTED_MENU_TEMPO, CONVERTED_MENU_TEMPO});
      }
    }
  }
}

SCENARIO("START leads to the character selection from the first stage") {
  GIVEN("An open menu") {
    Menu menu;
    run(*menu.state, OPENING_FRAMES);

    WHEN("START is fired") {
      menu.controller.states.button = true;
      run(*menu.state, 1);
      menu.controller.states.button = false;
      const Exit exit = runToExit(*menu.state, 1000);

      THEN("The selection follows with the stage reached reset") {
        REQUIRE(exit.next == EngineStateId::CharacterSelectionSequence);
        REQUIRE(menu.session.registers[RO] == 0);
      }
    }
  }
}

namespace {

constexpr int BACKDROP_WIDTH = 368;
constexpr int BACKDROP_HEIGHT = 290;
constexpr int BOB_WIDTH = 16;
constexpr int BOB_HEIGHT = 8;

openfranko::src::systems::graphics::IndexedBitmap
patterned(int width, int height, int seed, int hotX, int hotY) {
  openfranko::src::systems::graphics::IndexedBitmap bitmap;
  bitmap.width = width;
  bitmap.height = height;
  bitmap.hotspotX = hotX;
  bitmap.hotspotY = hotY;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const int value = (x * 3 + y * 5 + seed) % 16;
      bitmap.pixels.push_back(
          static_cast<uint8_t>((x + y) % 4 == 0 ? 0 : value));
    }
  }
  for (int color = 0; color < 16; ++color) {
    bitmap.palette.push_back(static_cast<uint16_t>(color * 0x111));
  }
  return bitmap;
}

struct PaintedMenu {
  explicit PaintedMenu(bool showsSprites) {
    files.bitmaps["assets/03B8.bmp"] =
        patterned(BACKDROP_WIDTH, BACKDROP_HEIGHT, 7, 0, 0);
    for (int index = 0; index < MENU_IMAGES; ++index) {
      files.bitmaps[assets::imagePath("0034", index)] =
          patterned(BOB_WIDTH, BOB_HEIGHT, index, 3, 2);
    }
    monitor.sprites = showsSprites;
    session.version = GameVersion::V10;
    state.emplace(monitor, speaker, controller, files, options, session);
  }

  FakeMonitor monitor;
  FakeSpeaker speaker;
  ControllerSystem controller;
  FakeFiles files;
  GameOptions options;
  GameSession session;
  std::optional<MenuState> state;
};

} // namespace

SCENARIO("Menu bobs shown as sprites look the same as bobs drawn") {
  GIVEN("Two menus on the same pictures, one on a monitor that shows sprites") {
    PaintedMenu drawn(false);
    PaintedMenu sprited(true);

    WHEN("They open, start the credits and the hand moves both ways") {
      int spriteFrames = 0;
      int redraws = 0;
      const uint8_t *lastPixels = nullptr;
      for (int frame = 0; frame < OPENING_FRAMES + 280; ++frame) {
        const bool right = frame == OPENING_FRAMES + 100;
        const bool left = frame == OPENING_FRAMES + 200;
        for (PaintedMenu *menu : {&drawn, &sprited}) {
          menu->controller.states.right = right;
          menu->controller.states.left = left;
          menu->state->update();
        }
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

      THEN("The bobs went out as sprites over a backdrop drawn once") {
        REQUIRE(spriteFrames > 200);
        REQUIRE(redraws == 0);
      }
    }
  }
}
