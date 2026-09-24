#include "GameOverState.h"

namespace openfranko::src::engine::states::gameOver {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "gameOverFrame";

} // namespace

GameOverState::GameOverState(systems::VideoSystem &videoSystem,
                             systems::AudioSystem &audioSystem,
                             systems::ControllerSystem &controllerSystem)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem), m_scene(m_host) {
  m_videoSystem.createScreen(SCREEN, street::GameOverScene::WIDTH,
                             street::GameOverScene::HEIGHT);
  m_videoSystem.switchScreen(SCREEN);
}

GameOverState::~GameOverState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> GameOverState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  m_scene.compose(m_frame);
  m_videoSystem.updateFrameImage(FRAME, street::GameOverScene::WIDTH,
                                 street::GameOverScene::HEIGHT, m_frame);
  m_videoSystem.drawImage(FRAME, 0, 0);
  if (m_scene.isFinished()) {
    return EngineStateEnum::HighScore;
  }
  return std::nullopt;
}

const street::GameOverScene &GameOverState::scene() const { return m_scene; }

} // namespace openfranko::src::engine::states::gameOver
