#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include "../systems/audio/AudioSystem.h"
#include "../systems/graphics/VideoSystem.h"
#include "../systems/input/ControllerSystem.h"
#include "../systems/input/Platform.h"
#include "GameOptions.h"
#include "assets/Files.h"
#include "states/EngineState.h"
#include "states/shared/EngineStreetHost.h"
#include "street/session/GameSession.h"

#include <memory>

namespace openfranko {
namespace src {
namespace engine {

class Engine {
public:
  Engine();
  Engine(states::EngineStateId firstState,
         street::session::GameSession startingSession);
  ~Engine();

  bool isRunning();

  void update();
  void run();

private:
  void updateState();

  states::EngineStateId versionState(states::EngineStateId state) const;
  void switchState(states::EngineStateId nextState);
  states::shared::EngineStreetHost &makeStreetHost();

  systems::input::Platform m_platform;
  std::unique_ptr<assets::Files> m_files;
  systems::graphics::VideoSystem m_videoSystem;
  systems::audio::AudioSystem m_audioSystem;
  systems::input::ControllerSystem m_controllerSystem;
  GameOptions m_options;
  street::session::GameSession m_session;

  std::unique_ptr<states::shared::EngineStreetHost> m_streetHost;
  std::unique_ptr<states::EngineState> m_currentState;
  bool m_running;
  bool m_booting = false;
};

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ENGINE_H_
