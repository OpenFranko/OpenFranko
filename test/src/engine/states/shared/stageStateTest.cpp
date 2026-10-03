#include "../../../../../src/engine/states/shared/StageState.h"

#include "../../../../../src/systems/graphics/Display.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../street/scenes/FakeStreetHost.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <optional>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::shared;
using namespace openfranko::src::engine::street::scenes;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::src::systems::graphics;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::engine::street::scenes;
using namespace openfranko::test::src::systems::graphics;

namespace {

enum class StageOutcome { Playing, GameOver, Quit, Cleared };

class Script : public FakeStreetHost {
public:
  StageOutcome outcome = StageOutcome::Playing;
  std::vector<StreetInput> inputs;
  Display frame;
  Display upcomingFrame;
  bool sprites = false;
  int handOvers = 0;
};

class ScriptedStage {
public:
  using Outcome = StageOutcome;

  ScriptedStage(StreetHost &host, GameSession &, GameOptions &)
      : m_script(static_cast<Script &>(host)) {}

  void showSprites(bool on) { m_script.sprites = on; }

  void advance(const StreetInput &input) { m_script.inputs.push_back(input); }

  void handOver() { ++m_script.handOvers; }

  const Display &output() const { return m_script.frame; }

  const Display &upcomingOutput() const { return m_script.upcomingFrame; }

  Outcome outcome() const { return m_script.outcome; }

private:
  Script &m_script;
};

using ScriptedState = StageState<ScriptedStage, EngineStateId::Level2Car>;

Display stageFrame() {
  Display frame;
  frame.width = 320;
  frame.height = 10;
  frame.border = 0x0F0;
  return frame;
}

struct Stage {
  explicit Stage(GameVersion version = GameVersion::V10) {
    session.version = version;
    script.frame = stageFrame();
    state.emplace(monitor, script, controller, options, session);
  }

  FakeMonitor monitor;
  Script script;
  ControllerSystem controller;
  GameOptions options;
  GameSession session;
  std::optional<ScriptedState> state;
};

void hold(ControllerSystem &controller, Key key) {
  KeyEvent event;
  event.key = key;
  event.pressed = true;
  controller.receiveKey(event);
}

} // namespace

SCENARIO("A stage's outcome picks the next state") {
  GIVEN("A stage being played") {
    Stage stage;

    THEN("It stays while playing") {
      REQUIRE_FALSE(stage.state->update().has_value());
    }

    THEN("A game over goes to the graveyard") {
      stage.script.outcome = StageOutcome::GameOver;
      REQUIRE(stage.state->update() == EngineStateId::GameOver);
      REQUIRE(stage.script.handOvers == 0);
    }

    THEN("Quitting goes to the scores") {
      stage.script.outcome = StageOutcome::Quit;
      REQUIRE(stage.state->update() == EngineStateId::HighScore);
    }

    THEN("Clearing it hands the stage over and goes to the next one") {
      stage.script.outcome = StageOutcome::Cleared;
      REQUIRE(stage.state->update() == EngineStateId::Level2Car);
      REQUIRE(stage.script.handOvers == 1);
    }
  }
}

SCENARIO("Bobs become sprites only on monitors that show sprites") {
  GIVEN("A monitor without sprites") {
    Stage stage;

    THEN("The stage draws its bobs into the screen") {
      REQUIRE_FALSE(stage.script.sprites);
    }
  }

  GIVEN("A monitor that shows sprites") {
    Stage stage;
    stage.monitor.sprites = true;
    stage.state.emplace(stage.monitor, stage.script, stage.controller,
                        stage.options, stage.session);

    THEN("The stage hands its bobs over as sprites") {
      REQUIRE(stage.script.sprites);
    }
  }
}

SCENARIO("A monitor that reads the buffers live gets the upcoming frame") {
  GIVEN("A stage whose upcoming frame has another border") {
    Stage stage;
    stage.script.upcomingFrame = stageFrame();
    stage.script.upcomingFrame.border = 0xF00;

    WHEN("The monitor copies each frame") {
      run(*stage.state, 1);

      THEN("It is shown the current frame") {
        REQUIRE(stage.monitor.pixel(0, 0) == toArgb(0x0F0));
      }
    }

    WHEN("The monitor reads the buffers while the next frame is drawn") {
      stage.monitor.live = true;
      run(*stage.state, 1);

      THEN("It is shown the frame that the next update keeps on screen") {
        REQUIRE(stage.monitor.pixel(0, 0) == toArgb(0xF00));
      }
    }
  }
}

SCENARIO("The stage's frame is shown in the stage's standard") {
  GIVEN("A PAL stage") {
    Stage stage;
    run(*stage.state, 1);

    THEN("Its frame goes to the monitor") {
      REQUIRE(stage.monitor.shows == 1);
      REQUIRE(stage.monitor.width == 320);
      REQUIRE(stage.monitor.pixel(0, 0) == toArgb(0x0F0));
      REQUIRE_FALSE(stage.monitor.ntsc);
    }

    WHEN("NTSC is chosen during play") {
      stage.options.ntsc = true;
      run(*stage.state, 1);

      THEN("The monitor follows on the next frame") {
        REQUIRE(stage.monitor.ntsc);
      }
    }
  }
}

SCENARIO("The stage reads the controller as its version does") {
  GIVEN("Version 1.0") {
    Stage stage;
    hold(stage.controller, Key::Left);
    hold(stage.controller, Key::F3);
    stage.controller.update();
    run(*stage.state, 1);

    THEN("The joystick and the F3 press reach the stage") {
      REQUIRE(stage.script.inputs.size() == 1);
      REQUIRE(stage.script.inputs[0].joystick == JOY_LEFT);
      REQUIRE(stage.script.inputs[0].key == SystemKey::Pal);
    }
  }

  GIVEN("Version 1.2") {
    Stage stage(GameVersion::V12);
    hold(stage.controller, Key::F3);
    stage.controller.update();
    run(*stage.state, 1);

    THEN("The held F3 switches the music off") {
      REQUIRE(stage.script.inputs[0].key == SystemKey::MusicOff);
    }
  }
}
