#include "GameOverState.h"

#include "../shared/StreetVideo.h"

namespace openfranko::src::engine::states::gameOver {

GameOverState::GameOverState(systems::graphics::VideoSystem &videoSystem,
                             systems::audio::AudioSystem &audioSystem,
                             systems::input::ControllerSystem &controllerSystem,
                             const GameOptions &options,
                             street::session::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem, session.version), m_scene(m_host, session),
      m_rows(visibleRows(
          pictureLine(street::scenes::GameOverScene::DISPLAY_LINE,
                      options.ntsc),
          street::scenes::GameOverScene::SCREEN_HEIGHT, options.ntsc)) {
  m_videoSystem.setNtsc(options.ntsc);
}

std::optional<EngineStateId> GameOverState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  shared::showSceneFrame(m_videoSystem, m_scene.output(), m_rows);
  if (m_scene.isFinished()) {
    return EngineStateId::HighScore;
  }
  return std::nullopt;
}

const street::scenes::GameOverScene &GameOverState::scene() const {
  return m_scene;
}

} // namespace openfranko::src::engine::states::gameOver
