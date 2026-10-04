#ifndef ENGINE_STATES_ENDING_ENDINGSTATE_H_
#define ENGINE_STATES_ENDING_ENDINGSTATE_H_

#include "../../../systems/graphics/Monitor.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../AmigaDisplay.h"
#include "../../GameOptions.h"
#include "../../street/scenes/EndingScene.h"
#include "../../street/scenes/StreetHost.h"
#include "../../street/session/GameSession.h"
#include "../EngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace ending {

class EndingState : public EngineState {
public:
  EndingState(systems::graphics::Monitor &monitor,
              street::scenes::StreetHost &host,
              systems::input::ControllerSystem &controllerSystem,
              const GameOptions &options,
              street::session::GameSession &session);

  std::optional<EngineStateId> update() override;

  const street::scenes::EndingScene &scene() const;

private:
  systems::graphics::Monitor &m_monitor;
  systems::input::ControllerSystem &m_controllerSystem;
  street::scenes::EndingScene m_scene;
  VisibleRows m_rows;
  systems::graphics::Display m_frame;
};

} // namespace ending
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_ENDING_ENDINGSTATE_H_
