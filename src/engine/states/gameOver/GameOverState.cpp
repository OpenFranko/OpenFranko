#include "GameOverState.h"

#include "../level1/StreetControls.h"

namespace openfranko::src::engine::states::gameOver {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "gameOverFrame";

} // namespace

GameOverState::GameOverState(systems::VideoSystem &videoSystem,
                             systems::AudioSystem &audioSystem,
                             systems::ControllerSystem &controllerSystem,
                             effects::GameOptions &options)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_controllerSystem(controllerSystem), m_options(options),
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
    level1::restartMenuMusic(m_audioSystem, m_options);
    return EngineStateEnum::Menu;
  }
  return std::nullopt;
}

const street::GameOverScene &GameOverState::scene() const { return m_scene; }

} // namespace openfranko::src::engine::states::gameOver
