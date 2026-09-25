#include "Level3BossState.h"

#include "../level1/StreetControls.h"

namespace openfranko::src::engine::states::level3 {
namespace {

constexpr auto FRAME = "level3BossFrame";

} // namespace

Level3BossState::Level3BossState(systems::VideoSystem &videoSystem,
                                 systems::AudioSystem &audioSystem,
                                 systems::ControllerSystem &controllerSystem,
                                 effects::GameOptions &options,
                                 street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem),
      m_stage(m_host, session, options) {
  level1::openStageScreen(m_videoSystem, options);
}

Level3BossState::~Level3BossState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> Level3BossState::update() {
  m_stage.advance(level1::readStreetInput(m_controllerSystem));
  m_stage.compose(m_frame);
  level1::showStageFrame(m_videoSystem, FRAME, m_frame, m_options);

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
