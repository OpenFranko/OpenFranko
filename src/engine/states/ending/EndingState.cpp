#include "EndingState.h"

#include "../shared/StreetVideo.h"

namespace openfranko::src::engine::states::ending {

EndingState::EndingState(systems::graphics::Monitor &monitor,
                         street::scenes::StreetHost &host,
                         systems::input::ControllerSystem &controllerSystem,
                         const GameOptions &options,
                         street::session::GameSession &session)
    : m_monitor(monitor), m_controllerSystem(controllerSystem),
      m_scene(host, session, options.ntsc),
      m_rows(visibleRows(m_scene.displayLine(),
                         street::scenes::EndingScene::SCREEN_HEIGHT,
                         options.ntsc)) {
  m_monitor.setNtsc(options.ntsc);
  m_scene.showSprites(monitor.showsSprites());
}

std::optional<EngineStateId> EndingState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  shared::showSceneFrame(m_monitor,
                         m_monitor.readsBuffersLive() ? m_scene.upcomingOutput()
                                                      : m_scene.output(),
                         m_rows);
  if (m_scene.isFinished()) {
    return EngineStateId::HighScore;
  }
  return std::nullopt;
}

const street::scenes::EndingScene &EndingState::scene() const {
  return m_scene;
}

} // namespace openfranko::src::engine::states::ending
