#include "Level1BossState.h"

#include "StreetControls.h"

namespace openfranko::src::engine::states::level1 {
namespace {

constexpr int SCREEN = 0;
constexpr auto FRAME = "level1BossFrame";

} // namespace

Level1BossState::Level1BossState(systems::VideoSystem &videoSystem,
                                 systems::AudioSystem &audioSystem,
                                 systems::ControllerSystem &controllerSystem,
                                 effects::GameOptions &options,
                                 street::GameSession &session)
    : m_videoSystem(videoSystem), m_audioSystem(audioSystem),
      m_controllerSystem(controllerSystem), m_options(options),
      m_host(audioSystem), m_stage(m_host, session, options) {
  m_videoSystem.createScreen(SCREEN, street::FRAME_WIDTH, street::FRAME_HEIGHT);
  m_videoSystem.switchScreen(SCREEN);
}

Level1BossState::~Level1BossState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> Level1BossState::update() {
  m_stage.advance(readStreetInput(m_controllerSystem));
  m_stage.compose(m_frame);
  m_videoSystem.updateFrameImage(FRAME, street::FRAME_WIDTH,
                                 street::FRAME_HEIGHT, m_frame);
  m_videoSystem.drawImage(FRAME, 0, 0);

  switch (m_stage.outcome()) {
  case street::BossStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::BossStage::Outcome::Quit:
    restartMenuMusic(m_audioSystem, m_options);
    return EngineStateEnum::Menu;
  case street::BossStage::Outcome::Playing:
  case street::BossStage::Outcome::BossDefeated:
    break;
  }
  return std::nullopt;
}

const street::BossStage &Level1BossState::stage() const { return m_stage; }

} // namespace openfranko::src::engine::states::level1
