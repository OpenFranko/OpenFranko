#include "HighScoreState.h"

namespace openfranko::src::engine::states::highScore {
namespace {

void save(const street::core::HighScoreTable &table) {
  street::core::writeHighScoreFile(table,
                                   street::core::HighScoreTable::FILE_NAME);
}

} // namespace

HighScoreState::HighScoreState(systems::graphics::VideoSystem &videoSystem,
                               systems::audio::AudioSystem &audioSystem,
                               effects::core::GameOptions &options,
                               street::ui::GameSession &session)
    : m_videoSystem(videoSystem), m_host(audioSystem, session.version),
      m_scene(m_host, session, options, save),
      m_rows(effects::color::visibleRows(
          effects::color::pictureLine(
              street::scenes::HighScoreScene::DISPLAY_LINE, options.ntsc),
          street::scenes::HighScoreScene::HEIGHT, options.ntsc)) {
  m_videoSystem.setNtsc(options.ntsc);
}

std::optional<EngineStateEnum> HighScoreState::update() {
  m_scene.advance();
  systems::graphics::Display output = m_scene.output();
  systems::graphics::cropRows(output, m_rows.first, m_rows.count);
  m_videoSystem.show(output);

  switch (m_scene.outcome()) {
  case street::scenes::HighScoreScene::Outcome::Menu:
    return EngineStateEnum::Menu;
  case street::scenes::HighScoreScene::Outcome::Continue:
    return EngineStateEnum::Continue;
  case street::scenes::HighScoreScene::Outcome::Running:
    break;
  }
  return std::nullopt;
}

const street::scenes::HighScoreScene &HighScoreState::scene() const {
  return m_scene;
}

} // namespace openfranko::src::engine::states::highScore
