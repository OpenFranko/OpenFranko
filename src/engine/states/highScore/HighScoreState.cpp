#include "HighScoreState.h"

#include "../shared/StreetVideo.h"

#include <utility>

namespace openfranko::src::engine::states::highScore {

HighScoreState::HighScoreState(systems::graphics::Monitor &monitor,
                               street::scenes::StreetHost &host,
                               GameOptions &options,
                               street::session::GameSession &session,
                               street::scenes::HighScoreScene::Save save)
    : m_monitor(monitor), m_scene(host, session, options, std::move(save)),
      m_rows(visibleRows(
          pictureLine(street::scenes::HighScoreScene::DISPLAY_LINE,
                      options.ntsc),
          street::scenes::HighScoreScene::SCREEN_HEIGHT, options.ntsc)) {
  m_monitor.setNtsc(options.ntsc);
}

std::optional<EngineStateId> HighScoreState::update() {
  m_scene.advance();
  shared::showSceneFrame(m_monitor, m_scene.output(), m_rows);

  switch (m_scene.outcome()) {
  case street::scenes::HighScoreScene::Outcome::Menu:
    return EngineStateId::Menu;
  case street::scenes::HighScoreScene::Outcome::Continue:
    return EngineStateId::Continue;
  case street::scenes::HighScoreScene::Outcome::Running:
    break;
  }
  return std::nullopt;
}

const street::scenes::HighScoreScene &HighScoreState::scene() const {
  return m_scene;
}

} // namespace openfranko::src::engine::states::highScore
