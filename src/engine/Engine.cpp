#include "Engine.h"
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
#include "states/protectionCheck/ProtectionCheckState.h"
#include "states/titleAndStory/TitleAndStoryState.h"
#include "states/worldSoftware/WorldSoftwareState.h"

#include <chrono>
#include <thread>
#include <utility>

namespace openfranko::src::engine {
namespace {

systems::KeyMode keyMode(states::EngineStateEnum state) {
  switch (state) {
  case states::EngineStateEnum::Level1:
  case states::EngineStateEnum::Level1Boss:
  case states::EngineStateEnum::Level1Car:
  case states::EngineStateEnum::Level2:
  case states::EngineStateEnum::Level2Boss:
  case states::EngineStateEnum::Level2Car:
  case states::EngineStateEnum::Level3:
  case states::EngineStateEnum::Level3Boss:
    return systems::KeyMode::Game;
  case states::EngineStateEnum::HighScore:
    return systems::KeyMode::NameEntry;
  case states::EngineStateEnum::Mirage:
  case states::EngineStateEnum::WorldSoftware:
  case states::EngineStateEnum::KneeAnimation:
  case states::EngineStateEnum::TitleAndStory:
  case states::EngineStateEnum::ProtectionCheck:
  case states::EngineStateEnum::Menu:
  case states::EngineStateEnum::CharacterSelection:
  case states::EngineStateEnum::StageProtectionCheck:
  case states::EngineStateEnum::Ending:
  case states::EngineStateEnum::GameOver:
  case states::EngineStateEnum::Continue:
    break;
  }
  return systems::KeyMode::FrontEnd;
}

} // namespace

Engine::Engine()
    : Engine(states::EngineStateEnum::Mirage, street::GameSession{}) {}

Engine::Engine(states::EngineStateEnum firstState,
               street::GameSession startingSession)
    : session(std::move(startingSession)), running(true) {
  session.highScores =
      street::readHighScoreFile(street::HighScoreTable::FILE_NAME)
          .value_or(street::HighScoreTable());
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
  std::optional<states::EngineStateEnum> nextState = currentState->update();
  while (nextState) {
    switchState(*nextState);
    nextState = currentState->update();
  }
}

void Engine::switchState(states::EngineStateEnum nextState) {
  videoSystem.clear();
  currentState.reset();
  booting = nextState == states::EngineStateEnum::Mirage;
  controllerSystem.setKeyMode(keyMode(nextState));
  if (nextState != states::EngineStateEnum::Menu) {
    session.keyboard.permit();
  }

  switch (nextState) {
  case states::EngineStateEnum::Mirage:
    currentState = std::make_unique<states::mirage::MirageState>(videoSystem);
    break;
  case states::EngineStateEnum::WorldSoftware:
    currentState = std::make_unique<states::worldSoftware::WorldSoftwareState>(
        videoSystem, audioSystem);
    break;
  case states::EngineStateEnum::KneeAnimation:
    currentState = std::make_unique<states::kneeAnimation::KneeAnimationState>(
        videoSystem, audioSystem, controllerSystem);
    break;
  case states::EngineStateEnum::TitleAndStory:
    currentState = std::make_unique<states::titleAndStory::TitleAndStoryState>(
        videoSystem, controllerSystem);
    break;
  case states::EngineStateEnum::ProtectionCheck:
    currentState =
        std::make_unique<states::protectionCheck::ProtectionCheckState>(
            videoSystem, audioSystem, session.keyboard);
    break;
  case states::EngineStateEnum::Menu:
    currentState = std::make_unique<states::menu::MenuState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::CharacterSelection:
    currentState =
        std::make_unique<states::characterSelection::CharacterSelectionState>(
            videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::Level1:
    currentState = std::make_unique<states::level1::Level1State>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::Level1Boss:
    currentState = std::make_unique<states::level1::Level1BossState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::Level1Car:
    currentState = std::make_unique<states::level1::Level1CarState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::Level2:
    currentState = std::make_unique<states::level2::Level2State>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::Level2Boss:
    currentState = std::make_unique<states::level2::Level2BossState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::Level2Car:
    currentState = std::make_unique<states::level2::Level2CarState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::StageProtectionCheck:
    currentState =
        std::make_unique<states::protectionCheck::ProtectionCheckState>(
            videoSystem, audioSystem, session.keyboard,
            states::protectionCheck::ProtectionCheckState::Check::Stage3);
    break;
  case states::EngineStateEnum::Level3:
    currentState = std::make_unique<states::level3::Level3State>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::Level3Boss:
    currentState = std::make_unique<states::level3::Level3BossState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::Ending:
    currentState = std::make_unique<states::ending::EndingState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::GameOver:
    currentState = std::make_unique<states::gameOver::GameOverState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::HighScore:
    currentState = std::make_unique<states::highScore::HighScoreState>(
        videoSystem, audioSystem, options, session);
    break;
  case states::EngineStateEnum::Continue:
    currentState = std::make_unique<states::continueSelect::ContinueState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  }
}

void Engine::update() {
  controllerSystem.update();
  if (booting && controllerSystem.isDeleteHeld()) {
    session.highScores = street::HighScoreTable();
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
