#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include "../systems/audio/AudioSystem.h"
#include "../systems/graphics/VideoSystem.h"
#include "../systems/input/ControllerSystem.h"
#include "../systems/input/Platform.h"
#include "GameOptions.h"
#include "states/IEngineState.h"
#include "street/session/GameSession.h"

#include <memory>

namespace openfranko {
namespace src {
namespace engine {

class Engine {
public:
  Engine();
  Engine(states::EngineStateEnum firstState,
         street::session::GameSession startingSession);
  ~Engine();

  bool isRunning();

  void update();
  void run();

  int refreshRate() const;

private:
  void updateState();

  states::EngineStateEnum versionState(states::EngineStateEnum state) const;
  void switchState(states::EngineStateEnum nextState);

  systems::input::Platform platform;
  systems::graphics::VideoSystem videoSystem;
  systems::audio::AudioSystem audioSystem;
  systems::input::ControllerSystem controllerSystem;
  GameOptions options;
  street::session::GameSession session;

  std::unique_ptr<states::IEngineState> currentState;
  bool running;
  bool booting = false;
};

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ENGINE_H_