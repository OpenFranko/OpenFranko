#ifndef ENGINE_STATES_LEVEL2BOSSSTATE_H_
#define ENGINE_STATES_LEVEL2BOSSSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../effects/core/GameOptions.h"
#include "../../street/scenes/BossStage.h"
#include "../../street/ui/GameSession.h"
#include "../IEngineState.h"
#include "../shared/EngineStreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level2 {

class Level2BossState : public IEngineState {
public:
  Level2BossState(systems::VideoSystem &videoSystem,
                  systems::AudioSystem &audioSystem,
                  systems::ControllerSystem &controllerSystem,
                  effects::core::GameOptions &options,
                  street::ui::GameSession &session);

  std::optional<EngineStateEnum> update() override;

  const street::BossStage &stage() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  const effects::core::GameOptions &m_options;
  shared::EngineStreetHost m_host;
  street::BossStage m_stage;
};

} // namespace level2
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL2BOSSSTATE_H_
