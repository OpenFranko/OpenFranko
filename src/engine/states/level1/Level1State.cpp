#include "Level1State.h"

#include "StreetControls.h"

namespace openfranko::src::engine::states::level1 {

Level1State::Level1State(systems::VideoSystem &videoSystem,
                         systems::AudioSystem &audioSystem,
                         systems::ControllerSystem &controllerSystem,
                         effects::GameOptions &options,
                         street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem),
      m_stage(m_host, session, options) {}

std::optional<EngineStateEnum> Level1State::update() {
  m_stage.advance(readStreetInput(m_controllerSystem));
  m_stage.compose(m_frame);
  showStageFrame(m_videoSystem, m_frame, m_options);

  switch (m_stage.outcome()) {
  case street::StreetStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::StreetStage::Outcome::Quit:
    return EngineStateEnum::HighScore;
  case street::StreetStage::Outcome::LevelFinished:
    return EngineStateEnum::Level1Boss;
  case street::StreetStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::StreetStage &Level1State::stage() const { return m_stage; }

} // namespace openfranko::src::engine::states::level1
