#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include "../systems/audio/AudioSystem.h"
#include "../systems/graphics/VideoSystem.h"
#include "../systems/input/ControllerSystem.h"
#include "../systems/input/Platform.h"
#include "effects/core/GameOptions.h"
#include "states/IEngineState.h"
#include "street/ui/GameSession.h"

#include <memory>

namespace openfranko {
namespace src {
namespace engine {

class Engine {
public:
  Engine();
  Engine(states::EngineStateEnum firstState,
         street::ui::GameSession startingSession);
  ~Engine();

  bool isRunning();

  void update();
  void run();

  int refreshRate() const;

private:
  void updateState();

  states::EngineStateEnum versionState(states::EngineStateEnum state) const;
  void switchState(states::EngineStateEnum nextState);

  systems::Platform platform;
  systems::VideoSystem videoSystem;
  systems::AudioSystem audioSystem;
  systems::ControllerSystem controllerSystem;
  effects::core::GameOptions options;
  street::ui::GameSession session;

  std::unique_ptr<states::IEngineState> currentState;
  bool running;
  bool booting = false;
};

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ENGINE_H_