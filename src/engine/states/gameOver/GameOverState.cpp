#include "GameOverState.h"

namespace openfranko::src::engine::states::gameOver {

GameOverState::GameOverState(systems::VideoSystem &videoSystem,
                             systems::AudioSystem &audioSystem,
                             systems::ControllerSystem &controllerSystem,
                             const effects::GameOptions &options,
                             street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem), m_scene(m_host, session),
      m_rows(effects::visibleRows(
          effects::pictureLine(street::GameOverScene::DISPLAY_LINE,
                               options.ntsc),
          street::GameOverScene::HEIGHT, options.ntsc)) {
  m_videoSystem.setNtsc(options.ntsc);
}

std::optional<EngineStateEnum> GameOverState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  m_scene.compose(m_frame);
  m_videoSystem.show(m_frame.data() + static_cast<std::size_t>(m_rows.first) *
                                          street::GameOverScene::WIDTH,
                     street::GameOverScene::WIDTH, m_rows.count);
  if (m_scene.isFinished()) {
    return EngineStateEnum::HighScore;
  }
  return std::nullopt;
}

const street::GameOverScene &GameOverState::scene() const { return m_scene; }

} // namespace openfranko::src::engine::states::gameOver
