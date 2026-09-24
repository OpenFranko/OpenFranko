#ifndef ENGINE_STATES_LEVEL1CARSTATE_H_
#define ENGINE_STATES_LEVEL1CARSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/GameOptions.h"
#include "../../street/CarStage.h"
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

class Level1CarState : public IEngineState {
public:
  Level1CarState(systems::VideoSystem &videoSystem,
                 systems::AudioSystem &audioSystem,
                 systems::ControllerSystem &controllerSystem,
                 effects::GameOptions &options, street::GameSession &session);
  ~Level1CarState();

  std::optional<EngineStateEnum> update() override;

  const street::CarStage &stage() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  EngineStreetHost m_host;
  street::CarStage m_stage;
  std::vector<uint32_t> m_frame;
};

} // namespace level1
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL1CARSTATE_H_
