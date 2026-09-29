#include "ContinueState.h"

#include "../shared/StreetVideo.h"

namespace openfranko::src::engine::states::continueSelect {

ContinueState::ContinueState(systems::graphics::Monitor &monitor,
                             street::scenes::StreetHost &host,
                             systems::input::ControllerSystem &controllerSystem,
                             const GameOptions &options,
                             street::session::GameSession &session)
    : m_monitor(monitor), m_controllerSystem(controllerSystem),
      m_scene(host, session),
      m_rows(visibleRows(
          pictureLine(street::scenes::ContinueScene::DISPLAY_LINE,
                      options.ntsc),
          street::scenes::ContinueScene::SCREEN_HEIGHT, options.ntsc)) {
  m_monitor.setNtsc(options.ntsc);
}

std::optional<EngineStateId> ContinueState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  shared::showSceneFrame(m_monitor, m_scene.output(), m_rows);

  switch (m_scene.outcome()) {
  case street::scenes::ContinueScene::Outcome::Continue:
    return EngineStateId::CharacterSelectionSequence;
  case street::scenes::ContinueScene::Outcome::NewGame:
    return EngineStateId::Menu;
  case street::scenes::ContinueScene::Outcome::Choosing:
    break;
  }
  return std::nullopt;
}

const street::scenes::ContinueScene &ContinueState::scene() const {
  return m_scene;
}

} // namespace openfranko::src::engine::states::continueSelect
