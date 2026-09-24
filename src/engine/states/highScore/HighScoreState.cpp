#include "HighScoreState.h"

namespace openfranko::src::engine::states::highScore {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "highScoreFrame";

void save(const street::HighScoreTable &table) {
  street::writeHighScoreFile(table, street::HighScoreTable::FILE_NAME);
}

} // namespace

HighScoreState::HighScoreState(systems::VideoSystem &videoSystem,
                               systems::AudioSystem &audioSystem,
                               systems::ControllerSystem &controllerSystem,
                               effects::GameOptions &options,
                               street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem), m_scene(m_host, session, options, save) {
  m_videoSystem.createScreen(SCREEN, street::HighScoreScene::WIDTH,
                             street::HighScoreScene::HEIGHT);
  m_videoSystem.switchScreen(SCREEN);
}

HighScoreState::~HighScoreState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> HighScoreState::update() {
  m_scene.advance(m_controllerSystem.typedKey().value_or('\0'));
  m_scene.compose(m_frame);
  m_videoSystem.updateFrameImage(FRAME, street::HighScoreScene::WIDTH,
                                 street::HighScoreScene::HEIGHT, m_frame);
  m_videoSystem.drawImage(FRAME, 0, 0);

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
