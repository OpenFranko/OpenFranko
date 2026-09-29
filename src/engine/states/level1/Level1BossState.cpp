#include "Level1BossState.h"

#include "../shared/StreetControls.h"
#include "../shared/StreetVideo.h"

namespace openfranko::src::engine::states::level1 {

Level1BossState::Level1BossState(
    systems::graphics::VideoSystem &videoSystem,
    systems::audio::AudioSystem &audioSystem,
    systems::input::ControllerSystem &controllerSystem, GameOptions &options,
    street::session::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem, session.version),
      m_stage(m_host, session, options) {}

std::optional<EngineStateId> Level1BossState::update() {
  m_stage.advance(
      shared::readStreetInput(m_controllerSystem, m_host.version()));
  shared::showStageFrame(m_videoSystem, m_stage.output(), m_options);

  switch (m_stage.outcome()) {
  case street::scenes::BossStage::Outcome::GameOver:
    return EngineStateId::GameOver;
  case street::scenes::BossStage::Outcome::Quit:
    return EngineStateId::HighScore;
  case street::scenes::BossStage::Outcome::BossDefeated:
    return EngineStateId::Level1Car;
  case street::scenes::BossStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::scenes::BossStage &Level1BossState::stage() const {
  return m_stage;
}

} // namespace openfranko::src::engine::states::level1
