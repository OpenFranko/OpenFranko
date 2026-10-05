#include "../../../../../src/engine/states/menu/MenuState.h"

#include "../../../../../src/engine/MenuTempo.h"
#include "../../../../../src/engine/amal/Machine.h"
#include "../../../../../src/engine/assets/Assets.h"
#include "../../../../../src/engine/effects/sequences/MenuSequence.h"
#include "../../../../../src/systems/graphics/Display.h"
#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
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
  explicit Menu(GameVersion version = GameVersion::V10, bool ntsc = false,
                FakeFiles menuFiles = {})
      : files(std::move(menuFiles)) {
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
  explicit PaintedMenu(bool showsSprites, int bobWidth = BOB_WIDTH) {
    files.bitmaps["assets/03B8.bmp"] =
        patterned(BACKDROP_WIDTH, BACKDROP_HEIGHT, 7, 0, 0);
    for (int index = 0; index < MENU_IMAGES; ++index) {
      files.bitmaps[assets::imagePath("0034", index)] =
          patterned(bobWidth, BOB_HEIGHT, index, 3, 2);
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

SCENARIO("Menu bobs that cannot be sprites are drawn into the menu") {
  GIVEN("Bobs of an odd width, on monitors with and without sprites") {
    PaintedMenu drawn(false, BOB_WIDTH - 1);
    PaintedMenu sprited(true, BOB_WIDTH - 1);

    WHEN("The menu opens and the credits start") {
      for (PaintedMenu *menu : {&drawn, &sprited}) {
        run(*menu->state, OPENING_FRAMES + 20);
      }

      THEN("No layer carries sprites and both menus look the same") {
        for (const auto &layer : sprited.monitor.shown().layers) {
          REQUIRE_FALSE(layer.carriesSprites);
        }
        REQUIRE(sprited.monitor.frame() == drawn.monitor.frame());
      }
    }
  }
}

namespace {

void press(Menu &menu, bool ControllerSystem::ControllerStates::*direction,
           int waitAfter) {
  menu.controller.states.*direction = true;
  run(*menu.state, 1);
  menu.controller.states.*direction = false;
  run(*menu.state, waitAfter);
}

constexpr int HAND_WAIT = 9;
constexpr int MACH_WAIT = 39;

int openingFrames(GameVersion version) {
  return OPENING_FRAMES +
         (version == GameVersion::V12 ? MenuSequence::VERSION12_MUSIC_WAIT : 0);
}

} // namespace

SCENARIO("The music and bass icons switch the speaker at once") {
  GIVEN("An open menu with the music on and the bass filter off") {
    Menu menu;
    run(*menu.state, OPENING_FRAMES);

    WHEN("The music icon is fired") {
      press(menu, &ControllerSystem::ControllerStates::down, HAND_WAIT);
      press(menu, &ControllerSystem::ControllerStates::button, MACH_WAIT);

      THEN("The music is turned down to nothing") {
        REQUIRE_FALSE(menu.options.music);
        REQUIRE(menu.speaker.volumes == std::vector<int>{0});
        REQUIRE(menu.speaker.filters.empty());
      }

      AND_WHEN("The bass icon is fired, then the music icon again") {
        press(menu, &ControllerSystem::ControllerStates::down, HAND_WAIT);
        press(menu, &ControllerSystem::ControllerStates::button, MACH_WAIT);
        press(menu, &ControllerSystem::ControllerStates::up, HAND_WAIT);
        press(menu, &ControllerSystem::ControllerStates::button, MACH_WAIT);

        THEN("The filter goes on and the music comes back at full volume") {
          REQUIRE(menu.options.bass);
          REQUIRE(menu.options.music);
          REQUIRE(menu.speaker.filters == std::vector<bool>{true});
          REQUIRE(menu.speaker.volumes == std::vector<int>{0, 63});
        }
      }
    }
  }
}

SCENARIO("Words typed in the menu become cheat codes when the game starts") {
  const auto typeAndStart = [](Menu &menu, const std::string &typed) {
    for (const char key : typed) {
      menu.session.keyboard.press(key);
    }
    run(*menu.state, openingFrames(menu.session.version) + 1);
    menu.controller.states.button = true;
    run(*menu.state, 1);
    menu.controller.states.button = false;
    return runToExit(*menu.state, 1000);
  };

  GIVEN("Version 1.0 with cent and mutant typed as the menu opens") {
    Menu menu;
    const Exit exit = typeAndStart(menu, "centmutant");

    THEN("The game starts from the second stage with fifteen lives") {
      REQUIRE(exit.next == EngineStateId::CharacterSelectionSequence);
      REQUIRE(menu.session.registers[RO] == 1);
      REQUIRE(menu.session.registers[RG] == 15);
    }

    THEN("The keys came from the key buffer, not from text entry") {
      REQUIRE_FALSE(menu.state->isEnteringText());
    }
  }

  GIVEN("Version 1.2 with ceat typed as the menu opens") {
    Menu menu(GameVersion::V12);
    const Exit exit = typeAndStart(menu, "ceat");

    THEN("The game starts from the first stage with six lives") {
      REQUIRE(exit.next == EngineStateId::CharacterSelectionSequence);
      REQUIRE(menu.session.registers[RO] == 0);
      REQUIRE(menu.session.registers[RG] == 6);
    }
  }

  GIVEN("Version 1.0 with nothing typed") {
    Menu menu;
    menu.session.registers[RG] = 3;
    const Exit exit = typeAndStart(menu, "");

    THEN("The lives are left alone") {
      REQUIRE(exit.next == EngineStateId::CharacterSelectionSequence);
      REQUIRE(menu.session.registers[RG] == 3);
    }
  }
}

namespace {

constexpr int ATTRACT_WIDTH = 320;
constexpr int MENU_WIDTH = 368;
constexpr int ATTRACT_LIMIT = MenuSequence::ATTRACT_AFTER + 100;
constexpr uint16_t TITLE_COLOR = 0x0F0;
constexpr uint8_t TITLE_INK = 2;
constexpr uint8_t HISCORE_INK = 1;
constexpr uint8_t LETTER_INK = 7;

openfranko::src::systems::graphics::IndexedBitmap solid(int width, int height,
                                                        uint8_t ink) {
  openfranko::src::systems::graphics::IndexedBitmap bitmap;
  bitmap.width = width;
  bitmap.height = height;
  bitmap.pixels.assign(static_cast<std::size_t>(width * height), ink);
  bitmap.palette.assign(32, 0x000);
  bitmap.palette[TITLE_INK] = TITLE_COLOR;
  bitmap.palette[HISCORE_INK] = 0x00F;
  bitmap.palette[LETTER_INK] = 0xF00;
  return bitmap;
}

FakeFiles attractFiles() {
  FakeFiles files;
  files.bitmaps["assets/03BA.bmp"] = solid(ATTRACT_WIDTH, 256, TITLE_INK);
  files.bitmaps["assets/03B9.bmp"] = solid(ATTRACT_WIDTH, 256, HISCORE_INK);
  for (int letter = 0; letter < LETTER_IMAGES; ++letter) {
    files.bitmaps[assets::imagePath("0035", letter)] = solid(8, 8, LETTER_INK);
  }
  return files;
}

struct AttractMenu : Menu {
  AttractMenu() : Menu(GameVersion::V10, false, attractFiles()) {}

  int runUntilWidth(int width) {
    for (int frame = 0; frame < ATTRACT_LIMIT; ++frame) {
      state->update();
      if (monitor.width == width) {
        return frame;
      }
    }
    return ATTRACT_LIMIT;
  }

  std::size_t inkedPixels() const {
    const std::vector<uint32_t> &frame = monitor.frame();
    return static_cast<std::size_t>(
        std::count_if(frame.begin(), frame.end(), [&frame](uint32_t pixel) {
          return pixel != frame.front();
        }));
  }
};

} // namespace

SCENARIO("The attract shows the title, then the scores a row at a time") {
  GIVEN("A menu left alone until the attract starts") {
    AttractMenu menu;
    run(*menu.state, OPENING_FRAMES);
    const int waited = menu.runUntilWidth(ATTRACT_WIDTH);
    run(*menu.state, 20);

    THEN("The title picture is shown in its own colours") {
      REQUIRE(waited < ATTRACT_LIMIT);
      REQUIRE(menu.monitor.width == ATTRACT_WIDTH);
      REQUIRE(menu.monitor.pixel(160, 100) ==
              openfranko::src::systems::graphics::toArgb(TITLE_COLOR));
    }

    WHEN("The joystick is touched") {
      press(menu, &ControllerSystem::ControllerStates::right, 0);
      const int widthAfterTouch = menu.monitor.width;
      run(*menu.state, 1);
      const int widthAfterNext = menu.monitor.width;
      run(*menu.state, 1);

      THEN("The title stays up while its screen closes, then the menu is "
           "back") {
        REQUIRE(widthAfterTouch == ATTRACT_WIDTH);
        REQUIRE(widthAfterNext == ATTRACT_WIDTH);
        REQUIRE(menu.monitor.width == MENU_WIDTH);
      }

      AND_WHEN("The menu is left alone again") {
        const int idle = menu.runUntilWidth(ATTRACT_WIDTH);
        run(*menu.state, 20);
        const std::size_t firstRows = menu.inkedPixels();
        run(*menu.state, 40);
        const std::size_t moreRows = menu.inkedPixels();
        run(*menu.state, 70);
        const std::size_t allRows = menu.inkedPixels();

        THEN("The scores come up a row at a time over the score picture") {
          REQUIRE(idle < ATTRACT_LIMIT);
          REQUIRE(idle > MenuSequence::ATTRACT_AFTER);
          REQUIRE(menu.monitor.pixel(0, 0) !=
                  openfranko::src::systems::graphics::toArgb(TITLE_COLOR));
          REQUIRE(firstRows > 0);
          REQUIRE(moreRows > firstRows);
          REQUIRE(allRows > moreRows);
        }

        AND_WHEN("The joystick is touched once the rows are all up") {
          press(menu, &ControllerSystem::ControllerStates::right, 2);
          const int closedWidth = menu.monitor.width;
          const int again = menu.runUntilWidth(ATTRACT_WIDTH);
          run(*menu.state, 20);

          THEN("The menu comes back and the title is shown next") {
            REQUIRE(closedWidth == MENU_WIDTH);
            REQUIRE(again < ATTRACT_LIMIT);
            REQUIRE(menu.monitor.pixel(160, 100) ==
                    openfranko::src::systems::graphics::toArgb(TITLE_COLOR));
          }
        }
      }
    }
  }
}

namespace {

class SlowFiles : public FakeFiles {
public:
  static constexpr int STEPS = 400;

  std::unique_ptr<BitmapLoad> beginBitmap(const std::string &path) override;

  int steps = 0;
};

class SlowLoad : public openfranko::src::engine::assets::Files::BitmapLoad {
public:
  SlowLoad(SlowFiles &files, std::string path)
      : m_files(files), m_path(std::move(path)) {}

  bool
  step(openfranko::src::systems::graphics::IndexedBitmap &bitmap) override {
    ++m_files.steps;
    if (++m_taken < SlowFiles::STEPS) {
      return false;
    }
    bitmap = m_files.loadBitmap(m_path);
    return true;
  }

private:
  SlowFiles &m_files;
  std::string m_path;
  int m_taken = 0;
};

std::unique_ptr<SlowFiles::BitmapLoad>
SlowFiles::beginBitmap(const std::string &path) {
  return std::make_unique<SlowLoad>(*this, path);
}

} // namespace

SCENARIO("Attract pictures still loading when the attract is due are "
         "finished at once") {
  GIVEN("A menu whose attract pictures take many steps each") {
    FakeMonitor monitor;
    FakeSpeaker speaker;
    ControllerSystem controller;
    SlowFiles files;
    GameOptions options;
    GameSession session;
    MenuState state(monitor, speaker, controller, files, options, session);

    WHEN("The menu is left alone until the attract starts") {
      run(state, OPENING_FRAMES + MenuSequence::ATTRACT_AFTER + 10);

      THEN("Both pictures were finished and every letter read before it") {
        REQUIRE(files.steps == 2 * SlowFiles::STEPS);
        REQUIRE(files.wasLoaded("assets/03BA.bmp"));
        REQUIRE(files.wasLoaded("assets/03B9.bmp"));
        REQUIRE(files.wasLoaded(assets::imagePath("0035", LETTER_IMAGES - 1)));
        REQUIRE(monitor.width == ATTRACT_WIDTH);
      }
    }
  }
}
