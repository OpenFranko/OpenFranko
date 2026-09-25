#include "ContinueState.h"

namespace openfranko::src::engine::states::continueSelect {

ContinueState::ContinueState(systems::VideoSystem &videoSystem,
                             systems::AudioSystem &audioSystem,
                             systems::ControllerSystem &controllerSystem,
                             const effects::GameOptions &options,
                             street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem), m_scene(m_host, session),
      m_rows(effects::visibleRows(
          effects::pictureLine(street::ContinueScene::DISPLAY_LINE,
                               options.ntsc),
          street::ContinueScene::HEIGHT, options.ntsc)) {
  m_videoSystem.setNtsc(options.ntsc);
}

std::optional<EngineStateEnum> ContinueState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  m_scene.compose(m_frame);
  m_videoSystem.show(m_frame.data() + static_cast<std::size_t>(m_rows.first) *
                                          street::ContinueScene::WIDTH,
                     street::ContinueScene::WIDTH, m_rows.count);

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
