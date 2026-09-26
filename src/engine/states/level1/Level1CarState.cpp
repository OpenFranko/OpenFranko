#include "Level1CarState.h"

#include "StreetControls.h"

namespace openfranko::src::engine::states::level1 {

Level1CarState::Level1CarState(systems::VideoSystem &videoSystem,
                               systems::AudioSystem &audioSystem,
                               systems::ControllerSystem &controllerSystem,
                               effects::GameOptions &options,
                               street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem, session.version),
      m_stage(m_host, session, options) {}

std::optional<EngineStateEnum> Level1CarState::update() {
  m_stage.advance(readStreetInput(m_controllerSystem, m_host.version()));
  showStageFrame(m_videoSystem, m_stage.output(), m_options);

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
