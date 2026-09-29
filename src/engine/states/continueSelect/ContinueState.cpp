#include "ContinueState.h"

namespace openfranko::src::engine::states::continueSelect {

ContinueState::ContinueState(systems::graphics::VideoSystem &videoSystem,
                             systems::audio::AudioSystem &audioSystem,
                             systems::input::ControllerSystem &controllerSystem,
                             const effects::core::GameOptions &options,
                             street::ui::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem, session.version), m_scene(m_host, session),
      m_rows(effects::color::visibleRows(
          effects::color::pictureLine(
              street::scenes::ContinueScene::DISPLAY_LINE, options.ntsc),
          street::scenes::ContinueScene::HEIGHT, options.ntsc)) {
  m_videoSystem.setNtsc(options.ntsc);
}

std::optional<EngineStateEnum> ContinueState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  systems::graphics::Display output = m_scene.output();
  systems::graphics::cropRows(output, m_rows.first, m_rows.count);
  m_videoSystem.show(output);

  switch (m_scene.outcome()) {
  case street::scenes::ContinueScene::Outcome::Continue:
    return EngineStateEnum::CharacterSelection;
  case street::scenes::ContinueScene::Outcome::NewGame:
    return EngineStateEnum::Menu;
  case street::scenes::ContinueScene::Outcome::Choosing:
    break;
  }
  return std::nullopt;
}

const street::scenes::ContinueScene &ContinueState::scene() const {
  return m_scene;
}

} // namespace openfranko::src::engine::states::continueSelect
