#include "EndingState.h"

namespace openfranko::src::engine::states::ending {

EndingState::EndingState(systems::graphics::VideoSystem &videoSystem,
                         systems::audio::AudioSystem &audioSystem,
                         systems::input::ControllerSystem &controllerSystem,
                         const GameOptions &options,
                         street::session::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem, session.version),
      m_scene(m_host, session, options.ntsc),
      m_rows(visibleRows(m_scene.displayLine(),
                         street::scenes::EndingScene::HEIGHT, options.ntsc)) {
  m_videoSystem.setNtsc(options.ntsc);
}

std::optional<EngineStateId> EndingState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  systems::graphics::Display output = m_scene.output();
  systems::graphics::cropRows(output, m_rows.first, m_rows.count);
  m_videoSystem.show(output);
  if (m_scene.isFinished()) {
    return EngineStateId::HighScore;
  }
  return std::nullopt;
}

const street::scenes::EndingScene &EndingState::scene() const {
  return m_scene;
}

} // namespace openfranko::src::engine::states::ending
