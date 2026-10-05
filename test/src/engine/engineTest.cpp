#include "../../../src/engine/Engine.h"

#include "../../../src/engine/assets/RequiredFiles.h"
#include "../systems/HeadlessSdl.h"
#include "TemporaryWorkingDirectory.h"

#include <SDL2/SDL.h>
#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::test::src::engine;
using namespace openfranko::test::src::systems;

namespace {

constexpr auto MIRAGE_LOGO = "assets/03C3.bmp";
constexpr int LOGO_WIDTH = 368;
constexpr int LOGO_HEIGHT = 290;
constexpr int FRAMES_BEFORE_WHITE = 6;
constexpr int MIDDLE_X = 400;
constexpr int MIDDLE_Y = 300;
constexpr uint32_t BLACK_PIXEL = 0xFF000000;
constexpr uint32_t WHITE_PIXEL = 0xFFFFFFFF;
constexpr Uint32 LATE_CLOSE_MS = 2000;

std::string bitmap(int width, int height) {
  constexpr std::size_t COLORS = 2;
  constexpr std::size_t PALETTE_OFFSET = 14 + 40;
  constexpr std::size_t PIXEL_OFFSET = PALETTE_OFFSET + COLORS * 4;
  const std::size_t stride = (static_cast<std::size_t>(width) + 3) / 4 * 4;
  std::string file(PIXEL_OFFSET + stride * static_cast<std::size_t>(height),
                   '\1');
  std::fill(file.begin(), file.begin() + PIXEL_OFFSET, '\0');
  const auto put = [&file](std::size_t offset, std::size_t value) {
    for (std::size_t i = 0; i < 4; ++i) {
      file[offset + i] = static_cast<char>(value >> (8 * i));
    }
  };
  file[0] = 'B';
  file[1] = 'M';
  put(10, PIXEL_OFFSET);
  put(14, 40);
  put(18, static_cast<std::size_t>(width));
  put(22, static_cast<std::size_t>(height));
  file[28] = 8;
  put(46, COLORS);
  file[PALETTE_OFFSET + 4] = '\x7F';
  return file;
}

std::vector<std::string> writeGameData(GameVersion version,
                                       std::size_t missing = 0) {
  const std::vector<std::string> required = assets::requiredFiles(version);
  const std::size_t written = required.size() - missing;
  for (std::size_t index = 0; index < written; ++index) {
    writeFile(required[index], "");
  }
  return {required.begin() + static_cast<std::ptrdiff_t>(written),
          required.end()};
}

std::vector<std::string> firstFiles(const std::vector<std::string> &files,
                                    std::size_t count) {
  return {files.begin(), files.begin() + static_cast<std::ptrdiff_t>(count)};
}

std::string incompleteData(const std::string &version,
                           const std::vector<std::string> &listed,
                           std::size_t more) {
  std::string message = "The Franko " + version +
                        " game data is incomplete, these files are missing:";
  for (const std::string &path : listed) {
    message += "\n  " + path;
  }
  if (more != 0) {
    message += "\n  and " + std::to_string(more) + " more";
  }
  return message + "\nExtract the game data again with frankoExtract.";
}

Uint32 SDLCALL closeLate(Uint32, void *) {
  pushEvent(SDL_QUIT);
  return 0;
}

class CloseOnResize {
public:
  CloseOnResize() { SDL_AddEventWatch(watch, this); }
  ~CloseOnResize() { SDL_DelEventWatch(watch, this); }

  CloseOnResize(const CloseOnResize &) = delete;
  CloseOnResize &operator=(const CloseOnResize &) = delete;

  bool hasClosed() const { return m_closed; }

private:
  static int SDLCALL watch(void *closer, SDL_Event *event) {
    auto *self = static_cast<CloseOnResize *>(closer);
    if (event->type == SDL_WINDOWEVENT && !self->m_closed) {
      self->m_closed = true;
      pushEvent(SDL_QUIT);
    }
    return 1;
  }

  bool m_closed = false;
};

} // namespace

SCENARIO("The engine needs the game data in its working directory") {
  GIVEN("A working directory without game data") {
    const TemporaryWorkingDirectory directory("openFrankoEngineNoData");
    const HeadlessSdl sdl;

    THEN("It refuses to start and says where the data belongs") {
      REQUIRE_THROWS_WITH(
          Engine{},
          "No game data found. Run the game from the directory that holds\n"
          "assets.tar or the assets directory made by frankoExtract.");
    }
  }

  GIVEN("An empty assets directory") {
    const TemporaryWorkingDirectory directory("openFrankoEngineEmptyData");
    std::filesystem::create_directory("assets");
    const HeadlessSdl sdl;

    THEN("The first eight missing 1.0 files are listed and the rest counted") {
      const std::vector<std::string> required =
          assets::requiredFiles(GameVersion::V10);
      REQUIRE_THROWS_WITH(
          Engine{},
          incompleteData("1.0", firstFiles(required, 8), required.size() - 8));
    }
  }

  GIVEN("A 1.0 data set missing nine files") {
    const TemporaryWorkingDirectory directory("openFrankoEngineNineMissing");
    const std::vector<std::string> missing = writeGameData(GameVersion::V10, 9);
    const HeadlessSdl sdl;

    THEN("Eight are listed and one more is counted") {
      REQUIRE_THROWS_WITH(Engine{},
                          incompleteData("1.0", firstFiles(missing, 8), 1));
    }
  }

  GIVEN("A 1.2 data set missing eight files") {
    const TemporaryWorkingDirectory directory("openFrankoEngineEightMissing");
    const std::vector<std::string> missing = writeGameData(GameVersion::V12, 8);
    const HeadlessSdl sdl;

    THEN("All eight are listed under the 1.2 name") {
      REQUIRE_THROWS_WITH(Engine{}, incompleteData("1.2", missing, 0));
    }
  }
}

SCENARIO("The first intro depends on the game version") {
  GIVEN("A 1.0 data set whose files are all empty") {
    const TemporaryWorkingDirectory directory("openFrankoEngineEmpty10");
    writeGameData(GameVersion::V10);
    const HeadlessSdl sdl;

    THEN("Booting stops at the Mirage logo's picture") {
      REQUIRE_THROWS_WITH(Engine{}, "Not a bitmap: assets/03C3.bmp");
    }
  }

  GIVEN("A 1.2 data set whose files are all empty") {
    const TemporaryWorkingDirectory directory("openFrankoEngineEmpty12");
    writeGameData(GameVersion::V12);
    const HeadlessSdl sdl;

    THEN("Booting stops at the spider logo's picture") {
      REQUIRE_THROWS_WITH(Engine{}, "Not a bitmap: assets/p50.bmp");
    }
  }
}

SCENARIO("Only Franko 1.0 checks the code cards") {
  GIVEN("A 1.0 data set whose files are all empty") {
    const TemporaryWorkingDirectory directory("openFrankoEngineCards10");
    writeGameData(GameVersion::V10);
    const HeadlessSdl sdl;

    THEN("Starting at the code card check reads the empty cards") {
      REQUIRE_THROWS_WITH(Engine(states::EngineStateId::ProtectionCheck,
                                 street::session::GameSession{}),
                          "The code cards need 200 bytes");
    }

    THEN("Starting at the card check before stage 3 reads them too") {
      REQUIRE_THROWS_WITH(Engine(states::EngineStateId::StageProtectionCheck,
                                 street::session::GameSession{}),
                          "The code cards need 200 bytes");
    }
  }

  GIVEN("A 1.2 data set whose files are all empty") {
    const TemporaryWorkingDirectory directory("openFrankoEngineCards12");
    writeGameData(GameVersion::V12);
    const HeadlessSdl sdl;

    WHEN("The engine starts at the code card check") {
      Engine engine(states::EngineStateId::ProtectionCheck,
                    street::session::GameSession{});

      THEN("The high score table starts instead and loads its letters") {
        REQUIRE_THROWS_WITH(engine.update(), "Not a bitmap: assets/p53.bmp");
      }
    }

    WHEN("The engine starts at the card check before stage 3") {
      Engine engine(states::EngineStateId::StageProtectionCheck,
                    street::session::GameSession{});

      THEN("The street starts instead and loads its stage") {
        REQUIRE_THROWS_WITH(engine.update(),
                            "Failed to load bitmap: assets/p0/p0_1.bmp");
      }
    }
  }
}

SCENARIO("A 1.0 data set boots into the Mirage logo") {
  GIVEN("A booted engine with a Mirage logo picture") {
    const TemporaryWorkingDirectory directory("openFrankoEngineMirage");
    writeGameData(GameVersion::V10);
    writeFile(MIRAGE_LOGO, bitmap(LOGO_WIDTH, LOGO_HEIGHT));
    const HeadlessSdl sdl;
    Engine engine;

    WHEN("Six frames run") {
      for (int frame = 0; frame < FRAMES_BEFORE_WHITE; ++frame) {
        REQUIRE(engine.isRunning());
        engine.update();
      }

      THEN("The logo screen is still black") {
        REQUIRE(windowPixel(MIDDLE_X, MIDDLE_Y) == BLACK_PIXEL);
      }

      AND_WHEN("One more frame runs") {
        engine.update();

        THEN("The screen flashes white") {
          REQUIRE(windowPixel(MIDDLE_X, MIDDLE_Y) == WHITE_PIXEL);
        }
      }
    }

    WHEN("The window is closed") {
      pushEvent(SDL_QUIT);

      THEN("The engine stops for good") {
        REQUIRE_FALSE(engine.isRunning());
        REQUIRE_FALSE(engine.isRunning());
      }
    }

    WHEN("Alt+Return is pressed and the window is closed once it resizes") {
      const CloseOnResize closer;
      pushKey(SDL_KEYDOWN, SDL_SCANCODE_RETURN, SDLK_RETURN, KMOD_LALT);
      const SDL_TimerID late = SDL_AddTimer(LATE_CLOSE_MS, closeLate, nullptr);
      engine.run();
      SDL_RemoveTimer(late);

      THEN("run updated a frame, which went fullscreen, before stopping") {
        REQUIRE(closer.hasClosed());
        REQUIRE(isFullscreen());
        REQUIRE_FALSE(engine.isRunning());
      }
    }
  }
}
