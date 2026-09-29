#ifndef ENGINE_STATES_ENDING_ENDINGSTATE_H_
#define ENGINE_STATES_ENDING_ENDINGSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../AmigaDisplay.h"
#include "../../GameOptions.h"
#include "../../street/scenes/EndingScene.h"
#include "../../street/session/GameSession.h"
#include "../IEngineState.h"
#include "../shared/EngineStreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace ending {

class EndingState : public IEngineState {
public:
  EndingState(systems::graphics::VideoSystem &videoSystem,
              systems::audio::AudioSystem &audioSystem,
              systems::input::ControllerSystem &controllerSystem,
              const GameOptions &options,
              street::session::GameSession &session);

  std::optional<EngineStateEnum> update() override;

  const street::scenes::EndingScene &scene() const;

private:
  systems::graphics::VideoSystem &m_videoSystem;
  systems::input::ControllerSystem &m_controllerSystem;
  shared::EngineStreetHost m_host;
  street::scenes::EndingScene m_scene;
  VisibleRows m_rows;
};

} // namespace ending
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_ENDING_ENDINGSTATE_H_
