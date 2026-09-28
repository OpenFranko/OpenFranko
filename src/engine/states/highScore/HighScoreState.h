#ifndef ENGINE_STATES_HIGHSCORESTATE_H_
#define ENGINE_STATES_HIGHSCORESTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/color/AmigaDisplay.h"
#include "../../effects/core/GameOptions.h"
#include "../../street/ui/GameSession.h"
#include "../../street/scenes/HighScoreScene.h"
#include "../IEngineState.h"
#include "../shared/EngineStreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace highScore {

class HighScoreState : public IEngineState {
public:
  HighScoreState(systems::VideoSystem &videoSystem,
                 systems::AudioSystem &audioSystem,
                 effects::GameOptions &options, street::GameSession &session);

  std::optional<EngineStateEnum> update() override;

  const street::HighScoreScene &scene() const;

private:
  systems::VideoSystem &m_videoSystem;
  shared::EngineStreetHost m_host;
  street::HighScoreScene m_scene;
  effects::VisibleRows m_rows;
};

} // namespace highScore
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_HIGHSCORESTATE_H_
