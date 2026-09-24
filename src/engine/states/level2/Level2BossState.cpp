#include "Level2BossState.h"

#include "../level1/StreetControls.h"

namespace openfranko::src::engine::states::level2 {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "level2BossFrame";

} // namespace

Level2BossState::Level2BossState(systems::VideoSystem &videoSystem,
                                 systems::AudioSystem &audioSystem,
                                 systems::ControllerSystem &controllerSystem,
                                 effects::GameOptions &options,
                                 street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_host(audioSystem), m_stage(m_host, session, options) {
  m_videoSystem.createScreen(SCREEN, street::FRAME_WIDTH, street::FRAME_HEIGHT);
  m_videoSystem.switchScreen(SCREEN);
}

Level2BossState::~Level2BossState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> Level2BossState::update() {
  m_stage.advance(level1::readStreetInput(m_controllerSystem));
  m_stage.compose(m_frame);
  m_videoSystem.updateFrameImage(FRAME, street::FRAME_WIDTH,
                                 street::FRAME_HEIGHT, m_frame);
  m_videoSystem.drawImage(FRAME, 0, 0);

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
