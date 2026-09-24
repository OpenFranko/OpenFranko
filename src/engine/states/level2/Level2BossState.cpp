#include "Level2BossState.h"

#include "../level1/StreetControls.h"

namespace openfranko::src::engine::states::level2 {
namespace {

constexpr auto FRAME = "level2BossFrame";

} // namespace

Level2BossState::Level2BossState(systems::VideoSystem &videoSystem,
                                 systems::AudioSystem &audioSystem,
                                 systems::ControllerSystem &controllerSystem,
                                 effects::GameOptions &options,
                                 street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem),
      m_stage(m_host, session, options) {
  level1::openStageScreen(m_videoSystem, options);
}

Level2BossState::~Level2BossState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> Level2BossState::update() {
  m_stage.advance(level1::readStreetInput(m_controllerSystem));
  m_stage.compose(m_frame);
  level1::showStageFrame(m_videoSystem, FRAME, m_frame, m_options);

  switch (m_stage.outcome()) {
  case street::BossStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::BossStage::Outcome::Quit:
    return EngineStateEnum::HighScore;
  case street::BossStage::Outcome::BossDefeated:
    return EngineStateEnum::Level2Car;
  case street::BossStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::BossStage &Level2BossState::stage() const { return m_stage; }

} // namespace openfranko::src::engine::states::level2
