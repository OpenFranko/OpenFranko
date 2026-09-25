#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include "../systems/AudioSystem.h"
#include "../systems/ControllerSystem.h"
#include "../systems/VideoSystem.h"
#include "effects/GameOptions.h"
#include "states/IEngineState.h"
#include "street/GameSession.h"

#include <memory>

namespace openfranko {
namespace src {
namespace engine {

class Engine {
public:
  Engine();
  Engine(states::EngineStateEnum firstState,
         street::GameSession startingSession);
  ~Engine();

  bool isRunning();

  void update();
  void run();

  int refreshRate() const;

private:
  void updateState();

  void switchState(states::EngineStateEnum nextState);

  SDL_Event event;
  systems::VideoSystem videoSystem;
  systems::AudioSystem audioSystem;
  systems::ControllerSystem controllerSystem;
  effects::GameOptions options;
  street::GameSession session;

  std::unique_ptr<states::IEngineState> currentState;
  bool running;
  bool booting = false;
};

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ENGINE_H_