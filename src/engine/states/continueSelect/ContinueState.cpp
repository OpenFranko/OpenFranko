#include "ContinueState.h"

namespace openfranko::src::engine::states::continueSelect {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "continueFrame";

} // namespace

ContinueState::ContinueState(systems::VideoSystem &videoSystem,
                             systems::AudioSystem &audioSystem,
                             systems::ControllerSystem &controllerSystem,
                             street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem), m_scene(m_host, session) {
  m_videoSystem.createScreen(SCREEN, street::ContinueScene::WIDTH,
                             street::ContinueScene::HEIGHT);
  m_videoSystem.switchScreen(SCREEN);
}

ContinueState::~ContinueState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> ContinueState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  m_scene.compose(m_frame);
  m_videoSystem.updateFrameImage(FRAME, street::ContinueScene::WIDTH,
                                 street::ContinueScene::HEIGHT, m_frame);
  m_videoSystem.drawImage(FRAME, 0, 0);

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
