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

namespace openfranko::src::engine {

Engine::Engine()
    : currentState(std::make_unique<states::mirage::MirageState>(videoSystem)),
      running(true) {
  session.highScores =
      street::readHighScoreFile(street::HighScoreTable::FILE_NAME)
          .value_or(street::HighScoreTable());
}

Engine::~Engine() {
  currentState.reset();
  SDL_Quit();
}

bool Engine::isRunning() {
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT)
      running = false;
  }
  return running;
}

void Engine::updateState() {
  if (currentState) {
    auto nextState = currentState->update();
    if (nextState) {
      switchState(nextState.value());
    }
  }
}

void Engine::switchState(states::EngineStateEnum nextState) {
  currentState.reset();

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
            videoSystem, audioSystem, controllerSystem);
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
            videoSystem, audioSystem, controllerSystem,
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
        videoSystem, audioSystem, controllerSystem, session);
    break;
  case states::EngineStateEnum::GameOver:
    currentState = std::make_unique<states::gameOver::GameOverState>(
        videoSystem, audioSystem, controllerSystem);
    break;
  case states::EngineStateEnum::HighScore:
    currentState = std::make_unique<states::highScore::HighScoreState>(
        videoSystem, audioSystem, controllerSystem, options, session);
    break;
  case states::EngineStateEnum::Continue:
    currentState = std::make_unique<states::continueSelect::ContinueState>(
        videoSystem, audioSystem, controllerSystem, session);
    break;
  }
}

void Engine::update() {
  controllerSystem.update();
  audioSystem.update();
  updateState();
  videoSystem.sync();
}

} // namespace openfranko::src::engine
