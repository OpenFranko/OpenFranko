#include "Level1CarState.h"

#include "StreetControls.h"

namespace openfranko::src::engine::states::level1 {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "level1CarFrame";

} // namespace

Level1CarState::Level1CarState(systems::VideoSystem &videoSystem,
                               systems::AudioSystem &audioSystem,
                               systems::ControllerSystem &controllerSystem,
                               effects::GameOptions &options,
                               street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem), m_stage(m_host, session, options) {
  m_videoSystem.createScreen(SCREEN, street::FRAME_WIDTH, street::FRAME_HEIGHT);
  m_videoSystem.switchScreen(SCREEN);
}

Level1CarState::~Level1CarState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> Level1CarState::update() {
  m_stage.advance(readStreetInput(m_controllerSystem));
  m_stage.compose(m_frame);
  m_videoSystem.updateFrameImage(FRAME, street::FRAME_WIDTH,
                                 street::FRAME_HEIGHT, m_frame);
  m_videoSystem.drawImage(FRAME, 0, 0);

  switch (m_stage.outcome()) {
  case street::CarStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::CarStage::Outcome::Quit:
    return EngineStateEnum::HighScore;
  case street::CarStage::Outcome::DriveFinished:
    return EngineStateEnum::Level2;
  case street::CarStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::CarStage &Level1CarState::stage() const { return m_stage; }

} // namespace openfranko::src::engine::states::level1
