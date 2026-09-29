#include "../../../../../src/engine/states/ending/EndingState.h"

#include "../../../../../src/engine/street/core/DoubleBuffer.h"
#include "../../../../../src/engine/street/core/IndexedSurface.h"
#include "../../../../../src/engine/street/ui/StageFrame.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../street/scenes/FakeStreetHost.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <optional>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::ending;
using namespace openfranko::src::engine::street::core;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::src::engine::street::ui;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::engine::street::scenes;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr int DANCE_SET = 0x38;
constexpr int STAGE_TOP = 47;

BossExit bossExit() {
  return {DoubleBuffer(IndexedSurface(320, 222)), levelPalette(false),
          STAGE_TOP, 0, IndexedSurface(304, 48)};
}

struct Ending {
  explicit Ending(bool ntsc = false) {
    options.ntsc = ntsc;
    session.bossExit.emplace(bossExit());
    state.emplace(monitor, host, controller, options, session);
  }

  FakeMonitor monitor;
  FakeStreetHost host;
  ControllerSystem controller;
  GameOptions options;
  GameSession session;
  std::optional<EndingState> state;
};

} // namespace

SCENARIO("The ending is shown in the options' standard") {
  GIVEN("PAL") {
    Ending ending;
    run(*ending.state, 1);

    THEN("The 320 pixel screen from line 50 shows all 256 rows") {
      REQUIRE_FALSE(ending.monitor.ntsc);
      REQUIRE(ending.monitor.width == 320);
      REQUIRE(ending.monitor.height == 256);
    }
  }

  GIVEN("NTSC") {
    Ending ending(true);
    run(*ending.state, 1);

    THEN("The monitor is switched and the screen cut to NTSC's lines") {
      REQUIRE(ending.monitor.ntsc);
      REQUIRE(ending.monitor.height == 236);
    }
  }
}

SCENARIO("The ending plays through the street host") {
  GIVEN("The first frame after the last boss") {
    Ending ending;
    run(*ending.state, 1);

    THEN("The scene has taken the boss's screen from the session") {
      REQUIRE_FALSE(ending.session.bossExit.has_value());
    }

    THEN("After the Cls stall the tune stops and the dance set loads") {
      run(*ending.state, 3);
      REQUIRE(ending.host.musicStops == 1);
      REQUIRE(ending.host.spriteSets ==
              std::vector<std::pair<int, int>>{{DANCE_SET, 0}});
    }
  }

  GIVEN("The ending clicked through") {
    Ending ending;
    Exit exit;
    for (int frame = 0; frame < 20000 && !exit.next; frame += 50) {
      ending.controller.states.button = !ending.controller.states.button;
      exit = runToExit(*ending.state, 50);
    }

    THEN("The scores follow") {
      REQUIRE(exit.next == EngineStateId::HighScore);
    }
  }
}
