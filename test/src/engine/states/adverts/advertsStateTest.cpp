#include "../../../../../src/engine/states/adverts/AdvertsState.h"

#include "../../../../../src/engine/AmigaDisplay.h"
#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::adverts;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr int SLIDE_FRAMES =
    2 * AdvertsState::KLIKER_FRAMES + SCREEN_REOPEN_VBLS;
constexpr int CLOSE_FRAMES = 2 * SCREEN_CLOSE_VBLS + 1;

struct Adverts {
  FakeMonitor monitor;
  ControllerSystem controller;
  FakeFiles files;
  AdvertsState state{monitor, controller, files};
};

} // namespace

SCENARIO("The six advert slides are read over the first frames") {
  GIVEN("The adverts state") {
    Adverts adverts;

    THEN("Nothing is read before the first frame") {
      REQUIRE(adverts.files.loaded.empty());
    }

    WHEN("The screen has opened") {
      run(adverts.state, AdvertsState::SLIDES);

      THEN("Pictures p80 to p85 are read in order") {
        REQUIRE(adverts.files.loaded ==
                std::vector<std::string>{"assets/p80.bmp", "assets/p81.bmp",
                                         "assets/p82.bmp", "assets/p83.bmp",
                                         "assets/p84.bmp", "assets/p85.bmp"});
      }
    }
  }
}

SCENARIO("Each slide fades in and out, then the Presents screen follows") {
  GIVEN("The slides left alone") {
    Adverts adverts;
    const Exit exit = runToExit(adverts.state, 3000);

    THEN("Every slide gets both KLIKER pauses before the screens close") {
      REQUIRE(exit.next == EngineStateId::Presents);
      REQUIRE(exit.frames == SCREEN_OPEN_VBLS +
                                 AdvertsState::SLIDES * SLIDE_FRAMES -
                                 SCREEN_REOPEN_VBLS + CLOSE_FRAMES);
      REQUIRE(adverts.monitor.shows == exit.frames);
    }
  }

  GIVEN("The first frame") {
    Adverts adverts;
    run(adverts.state, 1);

    THEN("The 320 pixel screen is shown as PAL sees it from line 42") {
      REQUIRE(adverts.monitor.width == 320);
      REQUIRE(adverts.monitor.height == 256);
    }
  }

  GIVEN("Fire held from the start") {
    Adverts adverts;
    adverts.controller.states.button = true;
    const Exit exit = runToExit(adverts.state, 3000);

    THEN("The first slide closes at once") {
      REQUIRE(exit.next == EngineStateId::Presents);
      REQUIRE(exit.frames == SCREEN_OPEN_VBLS + CLOSE_FRAMES);
    }
  }
}
