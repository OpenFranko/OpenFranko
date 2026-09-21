#include "Engine.h"
#include "states/MirageState.h"
#include "states/WorldSoftwareState.h"

namespace openfranko::src::engine {

Engine::Engine()
    : currentState(
          std::make_unique<states::MirageState>(videoSystem, controllerSystem)),
      running(true) {
  videoSystem.createScreen(0, 320, 240);
  videoSystem.switchScreen(0);

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

void Engine::updateInternalEngine() {
  if (currentState) {
    auto nextState = currentState->update();
    if (nextState) {
      switchState(nextState.value());
    }
  }
}

void Engine::switchState(EngineStateEnum nextState) {
  currentState.reset();

  switch (nextState) {
  case EngineStateEnum::Mirage:
    currentState =
        std::make_unique<states::MirageState>(videoSystem, controllerSystem);
    break;
  case EngineStateEnum::WorldSoftware:
    currentState = std::make_unique<states::WorldSoftwareState>(videoSystem);
    break;
  }
}

void Engine::update() {
  controllerSystem.update();
  updateInternalEngine();
  videoSystem.sync();
}

} // namespace openfranko::src::engine
