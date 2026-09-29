#ifndef ENGINE_STATES_GAMEOVER_GAMEOVERSTATE_H_
#define ENGINE_STATES_GAMEOVER_GAMEOVERSTATE_H_

#include "../../../systems/graphics/Monitor.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../AmigaDisplay.h"
#include "../../GameOptions.h"
#include "../../street/scenes/GameOverScene.h"
#include "../../street/scenes/StreetHost.h"
#include "../EngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace gameOver {

class GameOverState : public EngineState {
public:
  GameOverState(systems::graphics::Monitor &monitor,
                street::scenes::StreetHost &host,
                systems::input::ControllerSystem &controllerSystem,
                const GameOptions &options,
                street::session::GameSession &session);

  std::optional<EngineStateId> update() override;

  const street::scenes::GameOverScene &scene() const;

private:
  systems::graphics::Monitor &m_monitor;
  systems::input::ControllerSystem &m_controllerSystem;
  street::scenes::GameOverScene m_scene;
  VisibleRows m_rows;
};

} // namespace gameOver
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_GAMEOVER_GAMEOVERSTATE_H_
