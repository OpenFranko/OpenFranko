#include "HighScoreState.h"

namespace openfranko::src::engine::states::highScore {
namespace {

void save(const street::HighScoreTable &table) {
  street::writeHighScoreFile(table, street::HighScoreTable::FILE_NAME);
}

} // namespace

HighScoreState::HighScoreState(systems::VideoSystem &videoSystem,
                               systems::AudioSystem &audioSystem,
                               effects::GameOptions &options,
                               street::GameSession &session)
    : m_videoSystem(videoSystem), m_host(audioSystem),
      m_scene(m_host, session, options, save),
      m_rows(effects::visibleRows(
          effects::pictureLine(street::HighScoreScene::DISPLAY_LINE,
                               options.ntsc),
          street::HighScoreScene::HEIGHT, options.ntsc)) {
  m_videoSystem.setNtsc(options.ntsc);
}

std::optional<EngineStateEnum> HighScoreState::update() {
  m_scene.advance();
  m_scene.compose(m_frame);
  m_videoSystem.show(m_frame.data() + static_cast<std::size_t>(m_rows.first) *
                                          street::HighScoreScene::WIDTH,
                     street::HighScoreScene::WIDTH, m_rows.count);

  switch (m_scene.outcome()) {
  case street::HighScoreScene::Outcome::Menu:
    return EngineStateEnum::Menu;
  case street::HighScoreScene::Outcome::Continue:
    return EngineStateEnum::Continue;
  case street::HighScoreScene::Outcome::Running:
    break;
  }
  return std::nullopt;
}

const street::HighScoreScene &HighScoreState::scene() const { return m_scene; }

} // namespace openfranko::src::engine::states::highScore
