#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include "../systems/AudioSystem.h"
#include "../systems/ControllerSystem.h"
#include "../systems/VideoSystem.h"
#include "IEngineState.h"

#include <memory>

namespace openfranko {
namespace src {
namespace engine {

class Engine {
public:
  Engine();
  ~Engine();

  bool isRunning();

  void update();

private:
  void updateInternalEngine();

  void switchState(EngineStateEnum nextState);

  SDL_Event event;
  systems::VideoSystem videoSystem;
  systems::AudioSystem audioSystem;
  systems::ControllerSystem controllerSystem;

  std::unique_ptr<IEngineState> currentState;
  bool running;
};

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ENGINE_H_