#ifndef ENGINE_STATES_LEVEL2STATE_H_
#define ENGINE_STATES_LEVEL2STATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/GameOptions.h"
#include "../../street/GameSession.h"
#include "../../street/StreetStage.h"
#include "../IEngineState.h"
#include "../level1/EngineStreetHost.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level2 {

class Level2State : public IEngineState {
public:
  Level2State(systems::VideoSystem &videoSystem,
              systems::AudioSystem &audioSystem,
              systems::ControllerSystem &controllerSystem,
              effects::GameOptions &options, street::GameSession &session);
  ~Level2State();

  std::optional<EngineStateEnum> update() override;

  const street::StreetStage &stage() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  const effects::GameOptions &m_options;
  level1::EngineStreetHost m_host;
  street::StreetStage m_stage;
  std::vector<uint32_t> m_frame;
};

} // namespace level2
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL2STATE_H_
