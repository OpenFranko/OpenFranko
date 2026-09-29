#include "../../../../../src/engine/states/mirage/MirageState.h"

#include "../../../../../src/engine/AmigaDisplay.h"
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
using namespace openfranko::src::engine::states::mirage;
using namespace openfranko::src::systems::graphics;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr auto PICTURE = "assets/03C3.bmp";
constexpr int WIDTH = 368;
constexpr int HEIGHT = 290;
constexpr uint16_t SKY = 0x08F;

constexpr int WHITE_START = 2 * SCREEN_OPEN_VBLS + SCREEN_CLOSE_VBLS;
constexpr int HOLD_START = WHITE_START + 5 + 15 * 5 + SCREEN_CLOSE_VBLS;
constexpr int FRAMES = HOLD_START + 200 + 70 + SCREEN_CLOSE_VBLS;

FakeMonitor monitorOn(bool ntsc) {
  FakeMonitor monitor;
  monitor.ntsc = ntsc;
  return monitor;
}

FakeFiles pictureFiles() {
  IndexedBitmap picture;
  picture.width = WIDTH;
  picture.height = HEIGHT;
  picture.pixels.assign(static_cast<std::size_t>(WIDTH) * HEIGHT, 1);
  picture.palette = {0x000, SKY};
  FakeFiles files;
  files.bitmaps[PICTURE] = picture;
  return files;
}

struct Mirage {
  explicit Mirage(bool ntsc = false) : monitor(monitorOn(ntsc)) {}

  FakeMonitor monitor;
  FakeFiles files = pictureFiles();
  MirageState state{monitor, files};
};

} // namespace

SCENARIO("The Mirage logo is read through the files") {
  GIVEN("The first state after boot") {
    Mirage mirage;

    THEN("Its picture is the only file read") {
      REQUIRE(mirage.files.loaded == std::vector<std::string>{PICTURE});
    }
  }
}

SCENARIO("FOTO opens the logo black, turns it white and fades it in") {
  GIVEN("A PAL monitor") {
    Mirage mirage;

    THEN("The screen is black until the white frame") {
      run(mirage.state, WHITE_START);
      REQUIRE(mirage.monitor.shows == WHITE_START);
      REQUIRE(mirage.monitor.pixel(100, 100) == toArgb(0x000));
      run(mirage.state, 1);
      REQUIRE(mirage.monitor.pixel(100, 100) == toArgb(0xFFF));
    }

    THEN("The picture's own colours are reached for the hold") {
      run(mirage.state, HOLD_START);
      REQUIRE(mirage.monitor.pixel(100, 100) == toArgb(SKY));
    }

    THEN("The 368 pixel screen shows the rows PAL has below line 30") {
      run(mirage.state, 1);
      REQUIRE(mirage.monitor.width == WIDTH);
      REQUIRE(mirage.monitor.height == 280);
    }
  }

  GIVEN("An NTSC monitor") {
    Mirage mirage(true);

    THEN("The shorter NTSC frame cuts the screen at line 261") {
      run(mirage.state, 1);
      REQUIRE(mirage.monitor.height == 232);
    }
  }
}

SCENARIO("World Software follows once FOTO has closed the screen") {
  GIVEN("The logo left alone") {
    Mirage mirage;
    const Exit exit = runToExit(mirage.state, 1000);

    THEN("Every frame of the sequence is shown first") {
      REQUIRE(exit.next == EngineStateId::WorldSoftware);
      REQUIRE(exit.frames == FRAMES);
      REQUIRE(mirage.monitor.shows == FRAMES);
    }
  }
}
