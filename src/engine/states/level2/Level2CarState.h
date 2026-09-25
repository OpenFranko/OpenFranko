#ifndef ENGINE_STATES_LEVEL2CARSTATE_H_
#define ENGINE_STATES_LEVEL2CARSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/GameOptions.h"
#include "../../street/CarStage.h"
#include "../../street/GameSession.h"
#include "../IEngineState.h"
#include "../level1/EngineStreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level2 {

class Level2CarState : public IEngineState {
public:
  Level2CarState(systems::VideoSystem &videoSystem,
                 systems::AudioSystem &audioSystem,
                 systems::ControllerSystem &controllerSystem,
                 effects::GameOptions &options, street::GameSession &session);

  std::optional<EngineStateEnum> update() override;

  const street::CarStage &stage() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  const effects::GameOptions &m_options;
  level1::EngineStreetHost m_host;
  street::CarStage m_stage;
};

} // namespace level2
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL2CARSTATE_H_
