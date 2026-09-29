#include "HighScoreState.h"

#include "../shared/StreetVideo.h"

namespace openfranko::src::engine::states::highScore {
namespace {

void save(const street::core::HighScoreTable &table) {
  street::core::writeHighScoreFile(table,
                                   street::core::HighScoreTable::FILE_NAME);
}

} // namespace

HighScoreState::HighScoreState(systems::graphics::VideoSystem &videoSystem,
                               systems::audio::AudioSystem &audioSystem,
                               GameOptions &options,
                               street::session::GameSession &session)
    : m_videoSystem(videoSystem), m_host(audioSystem, session.version),
      m_scene(m_host, session, options, save),
      m_rows(visibleRows(
          pictureLine(street::scenes::HighScoreScene::DISPLAY_LINE,
                      options.ntsc),
          street::scenes::HighScoreScene::SCREEN_HEIGHT, options.ntsc)) {
  m_videoSystem.setNtsc(options.ntsc);
}

std::optional<EngineStateId> HighScoreState::update() {
  m_scene.advance();
  shared::showSceneFrame(m_videoSystem, m_scene.output(), m_rows);

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
