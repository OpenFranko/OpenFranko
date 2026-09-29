#ifndef ENGINE_STATES_LEVEL2_LEVEL2STATE_H_
#define ENGINE_STATES_LEVEL2_LEVEL2STATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../GameOptions.h"
#include "../../street/scenes/StreetStage.h"
#include "../../street/session/GameSession.h"
#include "../EngineState.h"
#include "../shared/EngineStreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level2 {

class Level2State : public EngineState {
public:
  Level2State(systems::graphics::VideoSystem &videoSystem,
              systems::audio::AudioSystem &audioSystem,
              systems::input::ControllerSystem &controllerSystem,
              GameOptions &options, street::session::GameSession &session);

  std::optional<EngineStateId> update() override;

  const street::scenes::StreetStage &stage() const;

private:
  systems::graphics::VideoSystem &m_videoSystem;
  systems::input::ControllerSystem &m_controllerSystem;
  const GameOptions &m_options;
  shared::EngineStreetHost m_host;
  street::scenes::StreetStage m_stage;
};

} // namespace level2
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL2_LEVEL2STATE_H_
