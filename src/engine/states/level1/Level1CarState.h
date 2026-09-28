#ifndef ENGINE_STATES_LEVEL1CARSTATE_H_
#define ENGINE_STATES_LEVEL1CARSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/core/GameOptions.h"
#include "../../street/scenes/CarStage.h"
#include "../../street/ui/GameSession.h"
#include "../IEngineState.h"
#include "../shared/EngineStreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level1 {

class Level1CarState : public IEngineState {
public:
  Level1CarState(systems::VideoSystem &videoSystem,
                 systems::AudioSystem &audioSystem,
                 systems::ControllerSystem &controllerSystem,
                 effects::GameOptions &options, street::GameSession &session);

  std::optional<EngineStateEnum> update() override;

  const street::CarStage &stage() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  const effects::GameOptions &m_options;
  shared::EngineStreetHost m_host;
  street::CarStage m_stage;
};

} // namespace level1
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL1CARSTATE_H_
