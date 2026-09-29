#ifndef ENGINE_STATES_HIGHSCORE_HIGHSCORESTATE_H_
#define ENGINE_STATES_HIGHSCORE_HIGHSCORESTATE_H_

#include "../../../systems/graphics/Monitor.h"
#include "../../AmigaDisplay.h"
#include "../../GameOptions.h"
#include "../../street/scenes/HighScoreScene.h"
#include "../../street/scenes/StreetHost.h"
#include "../../street/session/GameSession.h"
#include "../EngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace highScore {

class HighScoreState : public EngineState {
public:
  HighScoreState(systems::graphics::Monitor &monitor,
                 street::scenes::StreetHost &host, GameOptions &options,
                 street::session::GameSession &session,
                 street::scenes::HighScoreScene::Save save);

  std::optional<EngineStateId> update() override;

  const street::scenes::HighScoreScene &scene() const;

private:
  systems::graphics::Monitor &m_monitor;
  street::scenes::HighScoreScene m_scene;
  VisibleRows m_rows;
};

} // namespace highScore
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_HIGHSCORE_HIGHSCORESTATE_H_
