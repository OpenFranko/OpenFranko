#include "EndingState.h"

namespace openfranko::src::engine::states::ending {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "endingFrame";

} // namespace

EndingState::EndingState(systems::VideoSystem &videoSystem,
                         systems::AudioSystem &audioSystem,
                         systems::ControllerSystem &controllerSystem,
                         street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem), m_scene(m_host, session) {
  m_videoSystem.setNtsc(false);
  m_videoSystem.createScreen(SCREEN, street::EndingScene::WIDTH,
                             street::EndingScene::HEIGHT);
  m_videoSystem.switchScreen(SCREEN);
}

EndingState::~EndingState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> EndingState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  m_scene.compose(m_frame);
  m_videoSystem.updateFrameImage(FRAME, street::EndingScene::WIDTH,
                                 street::EndingScene::HEIGHT, m_frame);
  m_videoSystem.drawImage(FRAME, 0, 0);
  if (m_scene.isFinished()) {
    return EngineStateEnum::HighScore;
  }
  return std::nullopt;
}

const street::EndingScene &EndingState::scene() const { return m_scene; }

} // namespace openfranko::src::engine::states::ending
