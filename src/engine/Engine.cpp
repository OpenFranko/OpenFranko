#include "Engine.h"
#include "states/kneeAnimation/KneeAnimationState.h"
#include "states/mirage/MirageState.h"
#include "states/protectionCheck/ProtectionCheckState.h"
#include "states/titleAndStory/TitleAndStoryState.h"
#include "states/worldSoftware/WorldSoftwareState.h"

namespace openfranko::src::engine {

Engine::Engine()
    : currentState(std::make_unique<states::mirage::MirageState>(videoSystem)),
      running(true) {}

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
  }
}

void Engine::update() {
  controllerSystem.update();
  updateState();
  videoSystem.sync();
}

} // namespace openfranko::src::engine
