#include "Engine.h"
#include "assets/Assets.h"
#include "states/adverts/AdvertsState.h"
#include "states/characterSelection/CharacterSelectionState.h"
#include "states/continueSelect/ContinueState.h"
#include "states/ending/EndingState.h"
#include "states/gameOver/GameOverState.h"
#include "states/highScore/HighScoreState.h"
#include "states/kneeAnimation/KneeAnimationState.h"
#include "states/level1/Level1BossState.h"
#include "states/level1/Level1CarState.h"
#include "states/level1/Level1State.h"
#include "states/level2/Level2BossState.h"
#include "states/level2/Level2CarState.h"
#include "states/level2/Level2State.h"
#include "states/level3/Level3BossState.h"
#include "states/level3/Level3State.h"
#include "states/menu/MenuState.h"
#include "states/mirage/MirageState.h"
#include "states/presents/PresentsState.h"
#include "states/protectionCheck/ProtectionCheckState.h"
#include "states/spiderLogo/SpiderLogoState.h"
#include "states/titleAndStory/TitleAndStoryState.h"
#include "states/worldSoftware/WorldSoftwareState.h"

#include <chrono>
#include <thread>
#include <utility>

namespace openfranko::src::engine {
namespace {

systems::input::KeyMode keyMode(states::EngineStateId state) {
  switch (state) {
  case states::EngineStateId::Level1:
  case states::EngineStateId::Level1Boss:
  case states::EngineStateId::Level1Car:
  case states::EngineStateId::Level2:
  case states::EngineStateId::Level2Boss:
  case states::EngineStateId::Level2Car:
  case states::EngineStateId::Level3:
  case states::EngineStateId::Level3Boss:
    return systems::input::KeyMode::Game;
  case states::EngineStateId::HighScore:
    return systems::input::KeyMode::NameEntry;
  case states::EngineStateId::Mirage:
  case states::EngineStateId::SpiderLogo:
  case states::EngineStateId::Adverts:
  case states::EngineStateId::Presents:
  case states::EngineStateId::WorldSoftware:
  case states::EngineStateId::KneeAnimation:
  case states::EngineStateId::TitleAndStory:
  case states::EngineStateId::ProtectionCheck:
  case states::EngineStateId::Menu:
  case states::EngineStateId::CharacterSelectionSequence:
  case states::EngineStateId::StageProtectionCheck:
  case states::EngineStateId::Ending:
  case states::EngineStateId::GameOver:
  case states::EngineStateId::Continue:
    break;
  }
  return systems::input::KeyMode::FrontEnd;
}

} // namespace

Engine::Engine()
    : Engine(states::EngineStateId::Mirage, street::session::GameSession{}) {}

Engine::Engine(states::EngineStateId firstState,
               street::session::GameSession startingSession)
    : session(std::move(startingSession)), running(true) {
  session.version = assets::detectVersion();
  session.highScores =
      street::core::readHighScoreFile(street::core::HighScoreTable::FILE_NAME)
          .value_or(street::core::HighScoreTable(session.version));
  switchState(firstState);
}

Engine::~Engine() { currentState.reset(); }

bool Engine::isRunning() {
  if (!platform.pollEvents(controllerSystem)) {
    running = false;
  }
  return running;
}

void Engine::updateState() {
  if (!currentState) {
    return;
  }
  std::optional<states::EngineStateId> nextState = currentState->update();
  while (nextState) {
    switchState(*nextState);
    nextState = currentState->update();
  }
}

states::EngineStateId Engine::versionState(states::EngineStateId state) const {
  if (session.version != GameVersion::V12) {
    return state;
  }
  switch (state) {
  case states::EngineStateId::Mirage:
    return states::EngineStateId::SpiderLogo;
  case states::EngineStateId::ProtectionCheck:
    return states::EngineStateId::HighScore;
  case states::EngineStateId::StageProtectionCheck:
    return states::EngineStateId::Level3;
  default:
    return state;
  }
}

void Engine::switchState(states::EngineStateId nextState) {
  nextState = versionState(nextState);
#ifdef SKIP_COPY_PROTECTION
  if (nextState == states::EngineStateId::ProtectionCheck) {
    nextState = states::EngineStateId::HighScore;
  } else if (nextState == states::EngineStateId::StageProtectionCheck) {
    nextState = states::EngineStateId::Level3;
  }
#endif
  videoSystem.clear();
  currentState.reset();
  booting = nextState == states::EngineStateId::Mirage;
  controllerSystem.setKeyMode(keyMode(nextState));
  if (nextState != states::EngineStateId::Menu) {
    session.keyboard.permit();
  }

  switch (nextState) {
  case states::EngineStateId::Mirage:
    currentState = std::make_unique<states::mirage::MirageState>(videoSystem);
    break;
  case states::EngineStateId::SpiderLogo:
    currentState = std::make_unique<states::spiderLogo::SpiderLogoState>(
        videoSystem, audioSystem);
    break;
  case states::EngineStateId::Adverts:
    currentState = std::make_unique<states::adverts::AdvertsState>(
        videoSystem, controllerSystem);
    break;
  case states::EngineStateId::Presents:
    currentState = std::make_unique<states::presents::PresentsState>(
        videoSystem, audioSystem, controllerSystem);
    break;
  case states::EngineStateId::WorldSoftware:
    currentState = std::make_unique<states::worldSoftware::WorldSoftwareState>(
        videoSystem, audioSystem);
    break;
  case states::EngineStateId::KneeAnimation:
    currentState = std::make_unique<states::kneeAnimation::KneeAnimationState>(
        videoSystem, audioSystem, controllerSystem, session.version);
    break;
  case states::EngineStateId::TitleAndStory:
    currentState = std::make_unique<states::titleAndStory::TitleAndStoryState>(
        videoSystem, audioSystem, controllerSystem, session.version);
    break;
  case states::EngineStateId::ProtectionCheck:
    currentState =
        std::make_unique<states::protectionCheck::ProtectionCheckState>(
            videoSystem, audioSystem, session.keyboard);
    break;
  case states::EngineStateId::Menu:
    currentState = std::make_unique<states::menu::MenuState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::CharacterSelectionSequence:
    currentState =
        std::make_unique<states::characterSelection::CharacterSelectionState>(
            videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::Level1:
    currentState = std::make_unique<states::level1::Level1State>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::Level1Boss:
    currentState = std::make_unique<states::level1::Level1BossState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::Level1Car:
    currentState = std::make_unique<states::level1::Level1CarState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::Level2:
    currentState = std::make_unique<states::level2::Level2State>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::Level2Boss:
    currentState = std::make_unique<states::level2::Level2BossState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::Level2Car:
    currentState = std::make_unique<states::level2::Level2CarState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::StageProtectionCheck:
    currentState =
        std::make_unique<states::protectionCheck::ProtectionCheckState>(
            videoSystem, audioSystem, session.keyboard,
            states::protectionCheck::ProtectionCheckState::Check::Stage3);
    break;
  case states::EngineStateId::Level3:
    currentState = std::make_unique<states::level3::Level3State>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::Level3Boss:
    currentState = std::make_unique<states::level3::Level3BossState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::Ending:
    currentState = std::make_unique<states::ending::EndingState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::GameOver:
    currentState = std::make_unique<states::gameOver::GameOverState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateId::HighScore:
    currentState = std::make_unique<states::highScore::HighScoreState>(
        videoSystem, audioSystem, options, session);
    break;
  case states::EngineStateId::Continue:
    currentState = std::make_unique<states::continueSelect::ContinueState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  }
}

void Engine::update() {
  controllerSystem.update();
  if (booting && controllerSystem.isDeleteHeld()) {
    session.highScores = street::core::HighScoreTable(session.version);
  }
  for (const char key : controllerSystem.typedKeys()) {
    session.keyboard.press(key);
  }
  audioSystem.update();
  updateState();
  audioSystem.setVblRate(videoSystem.refreshRate());
  videoSystem.sync();
}

void Engine::run() {
  using Clock = std::chrono::steady_clock;
  Clock::time_point nextFrame = Clock::now();

  while (isRunning()) {
    update();

    const auto frameTime = std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(1.0 / refreshRate()));
    nextFrame += frameTime;
    const Clock::time_point now = Clock::now();
    if (nextFrame > now) {
      std::this_thread::sleep_until(nextFrame);
    } else if (now - nextFrame > frameTime) {
      nextFrame = now;
    }
  }
}

int Engine::refreshRate() const { return videoSystem.refreshRate(); }

} // namespace openfranko::src::engine
