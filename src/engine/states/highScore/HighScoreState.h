#ifndef ENGINE_STATES_HIGHSCORESTATE_H_
#define ENGINE_STATES_HIGHSCORESTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/GameOptions.h"
#include "../../street/GameSession.h"
#include "../../street/HighScoreScene.h"
#include "../IEngineState.h"
#include "../level1/EngineStreetHost.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace highScore {

class HighScoreState : public IEngineState {
public:
  HighScoreState(systems::VideoSystem &videoSystem,
                 systems::AudioSystem &audioSystem,
                 systems::ControllerSystem &controllerSystem,
                 effects::GameOptions &options, street::GameSession &session);
  ~HighScoreState();

  std::optional<EngineStateEnum> update() override;

  const street::HighScoreScene &scene() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  level1::EngineStreetHost m_host;
  street::HighScoreScene m_scene;
  std::vector<uint32_t> m_frame;
};

} // namespace highScore
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_HIGHSCORESTATE_H_
