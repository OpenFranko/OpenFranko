#ifndef ENGINE_STATES_CONTINUESELECT_CONTINUESTATE_H_
#define ENGINE_STATES_CONTINUESELECT_CONTINUESTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../AmigaDisplay.h"
#include "../../GameOptions.h"
#include "../../street/scenes/ContinueScene.h"
#include "../../street/session/GameSession.h"
#include "../IEngineState.h"
#include "../shared/EngineStreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace continueSelect {

class ContinueState : public IEngineState {
public:
  ContinueState(systems::graphics::VideoSystem &videoSystem,
                systems::audio::AudioSystem &audioSystem,
                systems::input::ControllerSystem &controllerSystem,
                const GameOptions &options,
                street::session::GameSession &session);

  std::optional<EngineStateEnum> update() override;

  const street::scenes::ContinueScene &scene() const;

private:
  systems::graphics::VideoSystem &m_videoSystem;
  systems::input::ControllerSystem &m_controllerSystem;
  shared::EngineStreetHost m_host;
  street::scenes::ContinueScene m_scene;
  VisibleRows m_rows;
};

} // namespace continueSelect
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_CONTINUESELECT_CONTINUESTATE_H_
