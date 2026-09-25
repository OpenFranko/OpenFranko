#include "EndingState.h"

namespace openfranko::src::engine::states::ending {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "endingFrame";

} // namespace

EndingState::EndingState(systems::VideoSystem &videoSystem,
                         systems::AudioSystem &audioSystem,
                         systems::ControllerSystem &controllerSystem,
                         const effects::GameOptions &options,
                         street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem), m_scene(m_host, session, options.ntsc),
      m_rows(effects::visibleRows(m_scene.displayLine(),
                                  street::EndingScene::HEIGHT, options.ntsc)) {
  m_videoSystem.setNtsc(options.ntsc);
  m_videoSystem.createScreen(SCREEN, street::EndingScene::WIDTH, m_rows.count);
  m_videoSystem.switchScreen(SCREEN);
}

EndingState::~EndingState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> EndingState::update() {
  m_scene.advance(m_controllerSystem.joystick());
  m_scene.compose(m_frame);
  m_videoSystem.updateFrameImage(FRAME, street::EndingScene::WIDTH,
                                 street::EndingScene::HEIGHT, m_frame);
  m_videoSystem.drawImage(FRAME, 0, -m_rows.first);
  if (m_scene.isFinished()) {
    return EngineStateEnum::HighScore;
  }
  return std::nullopt;
}

const street::EndingScene &EndingState::scene() const { return m_scene; }

} // namespace openfranko::src::engine::states::ending
