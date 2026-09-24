#include "Level1BossState.h"

#include "StreetControls.h"

namespace openfranko::src::engine::states::level1 {
namespace {

constexpr auto FRAME = "level1BossFrame";

} // namespace

Level1BossState::Level1BossState(systems::VideoSystem &videoSystem,
                                 systems::AudioSystem &audioSystem,
                                 systems::ControllerSystem &controllerSystem,
                                 effects::GameOptions &options,
                                 street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem),
      m_stage(m_host, session, options) {
  openStageScreen(m_videoSystem, options);
}

Level1BossState::~Level1BossState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> Level1BossState::update() {
  m_stage.advance(readStreetInput(m_controllerSystem));
  m_stage.compose(m_frame);
  showStageFrame(m_videoSystem, FRAME, m_frame, m_options);

  switch (m_stage.outcome()) {
  case street::BossStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::BossStage::Outcome::Quit:
    return EngineStateEnum::HighScore;
  case street::BossStage::Outcome::BossDefeated:
    return EngineStateEnum::Level1Car;
  case street::BossStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::BossStage &Level1BossState::stage() const { return m_stage; }

} // namespace openfranko::src::engine::states::level1
