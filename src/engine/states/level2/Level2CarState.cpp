#include "Level2CarState.h"

#include "../level1/StreetControls.h"

namespace openfranko::src::engine::states::level2 {
namespace {

constexpr auto FRAME = "level2CarFrame";

} // namespace

Level2CarState::Level2CarState(systems::VideoSystem &videoSystem,
                               systems::AudioSystem &audioSystem,
                               systems::ControllerSystem &controllerSystem,
                               effects::GameOptions &options,
                               street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem),
      m_stage(m_host, session, options) {
  level1::openStageScreen(m_videoSystem, options);
}

Level2CarState::~Level2CarState() { m_videoSystem.clearImage(FRAME); }

std::optional<EngineStateEnum> Level2CarState::update() {
  m_stage.advance(level1::readStreetInput(m_controllerSystem));
  m_stage.compose(m_frame);
  level1::showStageFrame(m_videoSystem, FRAME, m_frame, m_options);

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
