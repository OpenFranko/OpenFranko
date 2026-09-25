#include "Level3BossState.h"

#include "../level1/StreetControls.h"

namespace openfranko::src::engine::states::level3 {

Level3BossState::Level3BossState(systems::VideoSystem &videoSystem,
                                 systems::AudioSystem &audioSystem,
                                 systems::ControllerSystem &controllerSystem,
                                 effects::GameOptions &options,
                                 street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem),
      m_stage(m_host, session, options) {}

std::optional<EngineStateEnum> Level3BossState::update() {
  m_stage.advance(level1::readStreetInput(m_controllerSystem));
  level1::showStageFrame(m_videoSystem, m_stage.output(), m_options);

  switch (m_stage.outcome()) {
  case street::BossStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::BossStage::Outcome::Quit:
    return EngineStateEnum::HighScore;
  case street::BossStage::Outcome::BossDefeated:
    return EngineStateEnum::Ending;
  case street::BossStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::BossStage &Level3BossState::stage() const { return m_stage; }

} // namespace openfranko::src::engine::states::level3
