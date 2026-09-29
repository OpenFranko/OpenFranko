#include "Level2CarState.h"

#include "../shared/StreetControls.h"

namespace openfranko::src::engine::states::level2 {

Level2CarState::Level2CarState(systems::VideoSystem &videoSystem,
                               systems::AudioSystem &audioSystem,
                               systems::ControllerSystem &controllerSystem,
                               effects::core::GameOptions &options,
                               street::ui::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem, session.version),
      m_stage(m_host, session, options) {}

std::optional<EngineStateEnum> Level2CarState::update() {
  m_stage.advance(
      shared::readStreetInput(m_controllerSystem, m_host.version()));
  shared::showStageFrame(m_videoSystem, m_stage.output(), m_options);

  switch (m_stage.outcome()) {
  case street::scenes::CarStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::scenes::CarStage::Outcome::Quit:
    return EngineStateEnum::HighScore;
  case street::scenes::CarStage::Outcome::DriveFinished:
    return EngineStateEnum::StageProtectionCheck;
  case street::scenes::CarStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::scenes::CarStage &Level2CarState::stage() const {
  return m_stage;
}

} // namespace openfranko::src::engine::states::level2
