#include "Level2CarState.h"

#include "../level1/StreetControls.h"

namespace openfranko::src::engine::states::level2 {

Level2CarState::Level2CarState(systems::VideoSystem &videoSystem,
                               systems::AudioSystem &audioSystem,
                               systems::ControllerSystem &controllerSystem,
                               effects::GameOptions &options,
                               street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem),
      m_stage(m_host, session, options) {}

std::optional<EngineStateEnum> Level2CarState::update() {
  m_stage.advance(level1::readStreetInput(m_controllerSystem));
  level1::showStageFrame(m_videoSystem, m_stage.output(), m_options);

  switch (m_stage.outcome()) {
  case street::CarStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::CarStage::Outcome::Quit:
    return EngineStateEnum::HighScore;
  case street::CarStage::Outcome::DriveFinished:
    return EngineStateEnum::StageProtectionCheck;
  case street::CarStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::CarStage &Level2CarState::stage() const { return m_stage; }

} // namespace openfranko::src::engine::states::level2
