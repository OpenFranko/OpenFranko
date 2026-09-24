#ifndef ENGINE_STATES_LEVEL1BOSSSTATE_H_
#define ENGINE_STATES_LEVEL1BOSSSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/GameOptions.h"
#include "../../street/BossStage.h"
#include "../../street/GameSession.h"
#include "../IEngineState.h"
#include "EngineStreetHost.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level1 {

class Level1BossState : public IEngineState {
public:
  Level1BossState(systems::VideoSystem &videoSystem,
                  systems::AudioSystem &audioSystem,
                  systems::ControllerSystem &controllerSystem,
                  effects::GameOptions &options, street::GameSession &session);
  ~Level1BossState();

  std::optional<EngineStateEnum> update() override;

  const street::BossStage &stage() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::AudioSystem &m_audioSystem;
  systems::ControllerSystem &m_controllerSystem;
  effects::GameOptions &m_options;
  EngineStreetHost m_host;
  street::BossStage m_stage;
  std::vector<uint32_t> m_frame;
};

} // namespace level1
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL1BOSSSTATE_H_
