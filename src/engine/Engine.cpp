#include "Engine.h"
#include "states/mirage/MirageState.h"
#include "states/worldSoftware/WorldSoftwareState.h"

namespace openfranko::src::engine {

Engine::Engine()
    : currentState(std::make_unique<states::mirage::MirageState>(videoSystem)),
      running(true) {
  audioSystem.loadMusic("assets/0261.s3m");
  audioSystem.playMusic();
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
        videoSystem);
    break;
  }
}

void Engine::update() {
  controllerSystem.update();
  updateState();
  videoSystem.sync();
}

} // namespace openfranko::src::engine
