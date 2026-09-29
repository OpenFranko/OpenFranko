#ifndef ENGINE_STATES_HIGHSCORESTATE_H_
#define ENGINE_STATES_HIGHSCORESTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../effects/color/AmigaDisplay.h"
#include "../../effects/core/GameOptions.h"
#include "../../street/scenes/HighScoreScene.h"
#include "../../street/ui/GameSession.h"
#include "../IEngineState.h"
#include "../shared/EngineStreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace highScore {

class HighScoreState : public IEngineState {
public:
  HighScoreState(systems::graphics::VideoSystem &videoSystem,
                 systems::audio::AudioSystem &audioSystem,
                 effects::core::GameOptions &options,
                 street::ui::GameSession &session);

  std::optional<EngineStateEnum> update() override;

  const street::scenes::HighScoreScene &scene() const;

private:
  systems::graphics::VideoSystem &m_videoSystem;
  shared::EngineStreetHost m_host;
  street::scenes::HighScoreScene m_scene;
  effects::color::VisibleRows m_rows;
};

} // namespace highScore
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_HIGHSCORESTATE_H_
