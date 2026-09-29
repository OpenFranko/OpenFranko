#include "../../../../../src/engine/states/continueSelect/ContinueState.h"

#include "../../../../../src/engine/amal/Machine.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../street/scenes/FakeStreetHost.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <optional>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::amal;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::continueSelect;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::engine::street::scenes;
using namespace openfranko::test::src::systems::graphics;

namespace {

struct Choice {
  explicit Choice(bool ntsc = false) {
    options.ntsc = ntsc;
    session.stageReached = 1;
    session.registers[RO] = -1;
    state.emplace(monitor, host, controller, options, session);
  }

  void press(bool ControllerSystem::ControllerStates::*direction) {
    controller.states.*direction = true;
    run(*state, 1);
    controller.states.*direction = false;
  }

  FakeMonitor monitor;
  FakeStreetHost host;
  ControllerSystem controller;
  GameOptions options;
  GameSession session;
  std::optional<ContinueState> state;
};

} // namespace

SCENARIO("The continue screen is shown in the options' standard") {
  GIVEN("PAL") {
    Choice choice;
    run(*choice.state, 1);

    THEN("The 320 pixel screen from line 50 shows all 256 rows") {
      REQUIRE_FALSE(choice.monitor.ntsc);
      REQUIRE(choice.monitor.width == 320);
      REQUIRE(choice.monitor.height == 256);
    }
  }

  GIVEN("NTSC") {
    Choice choice(true);
    run(*choice.state, 1);

    THEN("The monitor is switched and the screen cut to NTSC's lines") {
      REQUIRE(choice.monitor.ntsc);
      REQUIRE(choice.monitor.height == 236);
    }
  }
}

SCENARIO("The joystick answers the continue question") {
  GIVEN("A player who died on stage 1") {
    Choice choice;
    run(*choice.state, 3);

    WHEN("TAK is fired") {
      choice.press(&ControllerSystem::ControllerStates::button);
      const Exit exit = runToExit(*choice.state, 1000);

      THEN("The character selection follows, one stage back") {
        REQUIRE(exit.next == EngineStateId::CharacterSelectionSequence);
        REQUIRE(choice.session.stageReached == 0);
      }
    }

    WHEN("NIE is fired") {
      choice.press(&ControllerSystem::ControllerStates::right);
      choice.press(&ControllerSystem::ControllerStates::button);
      const Exit exit = runToExit(*choice.state, 1000);

      THEN("The menu follows with the stage left alone") {
        REQUIRE(exit.next == EngineStateId::Menu);
        REQUIRE(choice.session.stageReached == 1);
      }
    }
  }
}
