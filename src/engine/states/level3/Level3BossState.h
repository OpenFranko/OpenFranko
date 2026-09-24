#ifndef ENGINE_STATES_LEVEL3BOSSSTATE_H_
#define ENGINE_STATES_LEVEL3BOSSSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/GameOptions.h"
#include "../../street/BossStage.h"
#include "../../street/GameSession.h"
#include "../IEngineState.h"
#include "../level1/EngineStreetHost.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level3 {

class Level3BossState : public IEngineState {
public:
  Level3BossState(systems::VideoSystem &videoSystem,
                  systems::AudioSystem &audioSystem,
                  systems::ControllerSystem &controllerSystem,
                  effects::GameOptions &options, street::GameSession &session);
  ~Level3BossState();

  std::optional<EngineStateEnum> update() override;

  const street::BossStage &stage() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  const effects::GameOptions &m_options;
  level1::EngineStreetHost m_host;
  street::BossStage m_stage;
  std::vector<uint32_t> m_frame;
};

} // namespace level3
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL3BOSSSTATE_H_
