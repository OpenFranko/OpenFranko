#include "Level1CarState.h"

#include "../shared/StreetControls.h"
#include "../shared/StreetVideo.h"

namespace openfranko::src::engine::states::level1 {

Level1CarState::Level1CarState(
    systems::graphics::VideoSystem &videoSystem,
    systems::audio::AudioSystem &audioSystem,
    systems::input::ControllerSystem &controllerSystem, GameOptions &options,
    street::session::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem, session.version),
      m_stage(m_host, session, options) {}

std::optional<EngineStateEnum> Level1CarState::update() {
  m_stage.advance(
      shared::readStreetInput(m_controllerSystem, m_host.version()));
  shared::showStageFrame(m_videoSystem, m_stage.output(), m_options);

  switch (m_stage.outcome()) {
  case street::scenes::CarStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::scenes::CarStage::Outcome::Quit:
    return EngineStateEnum::HighScore;
  case street::scenes::CarStage::Outcome::DriveFinished:
    return EngineStateEnum::Level2;
  case street::scenes::CarStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::scenes::CarStage &Level1CarState::stage() const {
  return m_stage;
}

} // namespace openfranko::src::engine::states::level1
