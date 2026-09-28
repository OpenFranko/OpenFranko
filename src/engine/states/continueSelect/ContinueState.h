#ifndef ENGINE_STATES_CONTINUESTATE_H_
#define ENGINE_STATES_CONTINUESTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../effects/color/AmigaDisplay.h"
#include "../../effects/core/GameOptions.h"
#include "../../street/scenes/ContinueScene.h"
#include "../../street/ui/GameSession.h"
#include "../IEngineState.h"
#include "../shared/EngineStreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace continueSelect {

class ContinueState : public IEngineState {
public:
  ContinueState(systems::VideoSystem &videoSystem,
                systems::AudioSystem &audioSystem,
                systems::ControllerSystem &controllerSystem,
                const effects::core::GameOptions &options,
                street::ui::GameSession &session);

  std::optional<EngineStateEnum> update() override;

  const street::ContinueScene &scene() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  shared::EngineStreetHost m_host;
  street::ContinueScene m_scene;
  effects::color::VisibleRows m_rows;
};

} // namespace continueSelect
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_CONTINUESTATE_H_
