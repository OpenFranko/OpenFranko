#include "EndingState.h"

namespace openfranko::src::engine::states::ending {

EndingState::EndingState(systems::VideoSystem &videoSystem,
                         systems::AudioSystem &audioSystem,
                         systems::ControllerSystem &controllerSystem,
                         const effects::GameOptions &options,
                         street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem, session.version),
      m_scene(m_host, session, options.ntsc),
      m_rows(effects::visibleRows(m_scene.displayLine(),
                                  street::EndingScene::HEIGHT, options.ntsc)) {
  m_videoSystem.setNtsc(options.ntsc);
}

std::optional<EngineStateEnum> EndingState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  systems::Display output = m_scene.output();
  systems::cropRows(output, m_rows.first, m_rows.count);
  m_videoSystem.show(output);
  if (m_scene.isFinished()) {
    return EngineStateEnum::HighScore;
  }
  return std::nullopt;
}

const street::EndingScene &EndingState::scene() const { return m_scene; }

} // namespace openfranko::src::engine::states::ending
