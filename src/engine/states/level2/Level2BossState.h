#ifndef ENGINE_STATES_LEVEL2_LEVEL2BOSSSTATE_H_
#define ENGINE_STATES_LEVEL2_LEVEL2BOSSSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../GameOptions.h"
#include "../../street/scenes/BossStage.h"
#include "../../street/session/GameSession.h"
#include "../IEngineState.h"
#include "../shared/EngineStreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level2 {

class Level2BossState : public IEngineState {
public:
  Level2BossState(systems::graphics::VideoSystem &videoSystem,
                  systems::audio::AudioSystem &audioSystem,
                  systems::input::ControllerSystem &controllerSystem,
                  GameOptions &options, street::session::GameSession &session);

  std::optional<EngineStateEnum> update() override;

  const street::scenes::BossStage &stage() const;

private:
  systems::graphics::VideoSystem &m_videoSystem;
  systems::input::ControllerSystem &m_controllerSystem;
  const GameOptions &m_options;
  shared::EngineStreetHost m_host;
  street::scenes::BossStage m_stage;
};

} // namespace level2
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL2_LEVEL2BOSSSTATE_H_
