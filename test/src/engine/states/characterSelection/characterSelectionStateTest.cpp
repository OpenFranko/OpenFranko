#include "../../../../../src/engine/states/characterSelection/CharacterSelectionState.h"

#include "../../../../../src/engine/amal/Machine.h"
#include "../../../../../src/engine/assets/Assets.h"
#include "../../../../../src/systems/audio/Mixer.h"
#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::amal;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::characterSelection;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::src::systems::audio;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::systems::audio;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr auto FRANKO = "characterFranko";
constexpr auto ALEX = "characterAlex";
constexpr auto FRANKO_PATH = "assets/0035/0035_sam1_8363Hz.wav";
constexpr auto ALEX_PATH = "assets/0035/0035_sam2_8363Hz.wav";

FakeFiles voiceFiles() {
  FakeFiles files;
  files.contents[FRANKO_PATH] = {};
  files.contents[ALEX_PATH] = {};
  return files;
}

struct Selection {
  explicit Selection(int stagesCleared = 0, bool music = false) {
    options.music = music;
    options.ntsc = true;
    session.nameScreenOpen = true;
    session.registers[RO] = static_cast<int16_t>(stagesCleared);
    state.emplace(monitor, speaker, controller, files, options, session);
  }

  void press(bool ControllerSystem::ControllerStates::*direction) {
    controller.states.*direction = true;
    run(*state, 1);
    controller.states.*direction = false;
  }

  FakeMonitor monitor;
  FakeSpeaker speaker;
  ControllerSystem controller;
  FakeFiles files = voiceFiles();
  GameOptions options;
  GameSession session;
  std::optional<CharacterSelectionState> state;
};

std::vector<int> fadeOutAndBack() {
  std::vector<int> volumes;
  for (int volume = 63; volume >= 0; --volume) {
    volumes.push_back(volume);
  }
  volumes.push_back(63);
  return volumes;
}

} // namespace

SCENARIO("The selection's picture, sprites and voices are loaded up front") {
  GIVEN("The selection") {
    Selection selection;

    THEN("The hiscore picture comes first, then three sprites of 0035") {
      REQUIRE(selection.files.loaded ==
              std::vector<std::string>{
                  "assets/03B9.bmp", assets::imagePath("0035", 0),
                  assets::imagePath("0035", 1), assets::imagePath("0035", 2)});
    }

    THEN("Both voices are found whatever their rates") {
      REQUIRE(selection.speaker.samples ==
              std::map<std::string, std::string>{{FRANKO, FRANKO_PATH},
                                                 {ALEX, ALEX_PATH}});
    }

    THEN("The monitor takes the options' standard") {
      REQUIRE(selection.monitor.ntsc);
    }

    WHEN("The state is left") {
      selection.state.reset();

      THEN("Both voices are cleared") {
        REQUIRE(selection.speaker.samples.empty());
      }
    }
  }
}

SCENARIO("The chosen character's voice is heard before the street") {
  GIVEN("Franko fired with the music off") {
    Selection selection;
    run(*selection.state, 1);
    selection.press(&ControllerSystem::ControllerStates::button);
    const Exit exit = runToExit(*selection.state, 1000);

    THEN("His voice plays on every voice and the music stops") {
      REQUIRE(exit.next == EngineStateId::Level1);
      REQUIRE(selection.speaker.plays ==
              std::vector<FakeSpeaker::Play>{{FRANKO, Mixer::ALL_VOICES}});
      REQUIRE(selection.speaker.musicStops == 1);
      REQUIRE(selection.speaker.volumes.empty());
      REQUIRE_FALSE(selection.session.nameScreenOpen);
    }
  }

  GIVEN("Alex fired with the music on") {
    Selection selection(0, true);
    run(*selection.state, 1);
    selection.press(&ControllerSystem::ControllerStates::right);
    selection.press(&ControllerSystem::ControllerStates::button);
    const Exit exit = runToExit(*selection.state, 1000);

    THEN("His voice plays and the music fades out before it stops") {
      REQUIRE(exit.next == EngineStateId::Level1);
      REQUIRE(selection.speaker.plays ==
              std::vector<FakeSpeaker::Play>{{ALEX, Mixer::ALL_VOICES}});
      REQUIRE(selection.speaker.volumes == fadeOutAndBack());
      REQUIRE(selection.speaker.musicStops == 1);
    }
  }
}

SCENARIO("The street is the one after the stages cleared") {
  GIVEN("One stage cleared") {
    Selection selection(1);
    run(*selection.state, 1);
    selection.press(&ControllerSystem::ControllerStates::button);

    THEN("Stage 2 follows") {
      REQUIRE(runToExit(*selection.state, 1000).next == EngineStateId::Level2);
    }
  }

  GIVEN("Two stages cleared") {
    Selection selection(2);
    run(*selection.state, 1);
    selection.press(&ControllerSystem::ControllerStates::button);

    THEN("The stage 3 code check follows") {
      REQUIRE(runToExit(*selection.state, 1000).next ==
              EngineStateId::StageProtectionCheck);
    }
  }
}
