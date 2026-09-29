#include "../../../../../src/engine/states/gameOver/GameOverState.h"

#include "../../../systems/graphics/FakeMonitor.h"
#include "../../street/scenes/FakeStreetHost.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <optional>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::gameOver;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::engine::street::scenes;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr int OBJECTS = 0x36;
constexpr int CLICK_FRAMES = 400;

struct Graveyard {
  explicit Graveyard(bool ntsc = false) {
    options.ntsc = ntsc;
    state.emplace(monitor, host, controller, options, session);
  }

  FakeMonitor monitor;
  FakeStreetHost host;
  ControllerSystem controller;
  GameOptions options;
  GameSession session;
  std::optional<GameOverState> state;
};

} // namespace

SCENARIO("Game over is shown in the options' standard") {
  GIVEN("PAL") {
    Graveyard graveyard;
    run(*graveyard.state, 1);

    THEN("The 368 pixel screen from line 45 shows all 256 rows") {
      REQUIRE_FALSE(graveyard.monitor.ntsc);
      REQUIRE(graveyard.monitor.width == 368);
      REQUIRE(graveyard.monitor.height == 256);
    }
  }

  GIVEN("NTSC") {
    Graveyard graveyard(true);
    run(*graveyard.state, 1);

    THEN("The monitor is switched and the screen cut to NTSC's lines") {
      REQUIRE(graveyard.monitor.ntsc);
      REQUIRE(graveyard.monitor.height == 236);
    }
  }
}

SCENARIO("Game over plays through the street host") {
  GIVEN("The first frame") {
    Graveyard graveyard;
    run(*graveyard.state, 1);

    THEN("The music is stopped and the objects are loading") {
      REQUIRE(graveyard.host.musicStops == 1);
      REQUIRE(graveyard.host.spriteSets ==
              std::vector<std::pair<int, int>>{{OBJECTS, 0}});
    }
  }
}

SCENARIO("The scores follow the graveyard, sooner if fire is pressed") {
  GIVEN("One graveyard left alone and one with fire held") {
    Graveyard idle;
    Graveyard fired;
    fired.controller.states.button = true;
    const Exit waited = runToExit(*idle.state, 5000);
    const Exit clicked = runToExit(*fired.state, 5000);

    THEN("Both go to the scores, fire cutting the KLIKER wait short") {
      REQUIRE(waited.next == EngineStateId::HighScore);
      REQUIRE(clicked.next == EngineStateId::HighScore);
      REQUIRE(waited.frames - clicked.frames == CLICK_FRAMES);
    }
  }
}
