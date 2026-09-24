#ifndef ENGINE_STATES_GAMEOVERSTATE_H_
#define ENGINE_STATES_GAMEOVERSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/GameOptions.h"
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
                systems::ControllerSystem &controllerSystem,
                effects::GameOptions &options);
  ~GameOverState();

  std::optional<EngineStateEnum> update() override;

  const street::GameOverScene &scene() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::AudioSystem &m_audioSystem;
  systems::ControllerSystem &m_controllerSystem;
  effects::GameOptions &m_options;
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
