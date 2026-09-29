#ifndef ENGINE_STATES_LEVEL2_LEVEL2CARSTATE_H_
#define ENGINE_STATES_LEVEL2_LEVEL2CARSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../GameOptions.h"
#include "../../street/scenes/CarStage.h"
#include "../../street/session/GameSession.h"
#include "../EngineState.h"
#include "../shared/EngineStreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level2 {

class Level2CarState : public EngineState {
public:
  Level2CarState(systems::graphics::VideoSystem &videoSystem,
                 systems::audio::AudioSystem &audioSystem,
                 systems::input::ControllerSystem &controllerSystem,
                 GameOptions &options, street::session::GameSession &session);

  std::optional<EngineStateId> update() override;

  const street::scenes::CarStage &stage() const;

private:
  systems::graphics::VideoSystem &m_videoSystem;
  systems::input::ControllerSystem &m_controllerSystem;
  const GameOptions &m_options;
  shared::EngineStreetHost m_host;
  street::scenes::CarStage m_stage;
};

} // namespace level2
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL2_LEVEL2CARSTATE_H_
