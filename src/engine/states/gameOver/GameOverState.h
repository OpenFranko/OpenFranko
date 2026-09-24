#ifndef ENGINE_STATES_GAMEOVERSTATE_H_
#define ENGINE_STATES_GAMEOVERSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../street/GameOverScene.h"
#include "../IEngineState.h"
#include "../level1/EngineStreetHost.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace gameOver {

class GameOverState : public IEngineState {
public:
  GameOverState(systems::VideoSystem &videoSystem,
                systems::AudioSystem &audioSystem,
                systems::ControllerSystem &controllerSystem);
  ~GameOverState();

  std::optional<EngineStateEnum> update() override;

  const street::GameOverScene &scene() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  level1::EngineStreetHost m_host;
  street::GameOverScene m_scene;
  std::vector<uint32_t> m_frame;
};

} // namespace gameOver
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_GAMEOVERSTATE_H_
