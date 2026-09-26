#include "ContinueState.h"

namespace openfranko::src::engine::states::continueSelect {

ContinueState::ContinueState(systems::VideoSystem &videoSystem,
                             systems::AudioSystem &audioSystem,
                             systems::ControllerSystem &controllerSystem,
                             const effects::GameOptions &options,
                             street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem, session.version), m_scene(m_host, session),
      m_rows(effects::visibleRows(
          effects::pictureLine(street::ContinueScene::DISPLAY_LINE,
                               options.ntsc),
          street::ContinueScene::HEIGHT, options.ntsc)) {
  m_videoSystem.setNtsc(options.ntsc);
}

std::optional<EngineStateEnum> ContinueState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  systems::Display output = m_scene.output();
  systems::cropRows(output, m_rows.first, m_rows.count);
  m_videoSystem.show(output);

  switch (m_scene.outcome()) {
  case street::ContinueScene::Outcome::Continue:
    return EngineStateEnum::CharacterSelection;
  case street::ContinueScene::Outcome::NewGame:
    return EngineStateEnum::Menu;
  case street::ContinueScene::Outcome::Choosing:
    break;
  }
  return std::nullopt;
}

const street::ContinueScene &ContinueState::scene() const { return m_scene; }

} // namespace openfranko::src::engine::states::continueSelect
