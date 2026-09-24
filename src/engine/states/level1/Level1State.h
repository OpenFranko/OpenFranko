#ifndef ENGINE_STATES_LEVEL1STATE_H_
#define ENGINE_STATES_LEVEL1STATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/GameOptions.h"
#include "../../street/GameSession.h"
#include "../../street/StreetStage.h"
#include "../IEngineState.h"
#include "EngineStreetHost.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level1 {

class Level1State : public IEngineState {
public:
  Level1State(systems::VideoSystem &videoSystem,
              systems::AudioSystem &audioSystem,
              systems::ControllerSystem &controllerSystem,
              effects::GameOptions &options, street::GameSession &session);
  ~Level1State();

  std::optional<EngineStateEnum> update() override;

  const street::StreetStage &stage() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  EngineStreetHost m_host;
  street::StreetStage m_stage;
  std::vector<uint32_t> m_frame;
};

} // namespace level1
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL1STATE_H_
