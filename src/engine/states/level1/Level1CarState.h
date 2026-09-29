#ifndef ENGINE_STATES_LEVEL1CARSTATE_H_
#define ENGINE_STATES_LEVEL1CARSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
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
  Level1CarState(systems::graphics::VideoSystem &videoSystem,
                 systems::audio::AudioSystem &audioSystem,
                 systems::input::ControllerSystem &controllerSystem,
                 effects::core::GameOptions &options,
                 street::ui::GameSession &session);

  std::optional<EngineStateEnum> update() override;

  const street::scenes::CarStage &stage() const;

private:
  systems::graphics::VideoSystem &m_videoSystem;
  systems::input::ControllerSystem &m_controllerSystem;
  const effects::core::GameOptions &m_options;
  shared::EngineStreetHost m_host;
  street::scenes::CarStage m_stage;
};

} // namespace level1
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL1CARSTATE_H_
