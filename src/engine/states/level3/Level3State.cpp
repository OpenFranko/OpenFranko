#include "Level3State.h"

#include "../level1/StreetControls.h"

namespace openfranko::src::engine::states::level3 {

Level3State::Level3State(systems::VideoSystem &videoSystem,
                         systems::AudioSystem &audioSystem,
                         systems::ControllerSystem &controllerSystem,
                         effects::GameOptions &options,
                         street::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem),
      m_stage(m_host, session, options) {}

std::optional<EngineStateEnum> Level3State::update() {
  m_stage.advance(level1::readStreetInput(m_controllerSystem));
  m_stage.compose(m_frame);
  level1::showStageFrame(m_videoSystem, m_frame, m_options);

  switch (m_stage.outcome()) {
  case street::StreetStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::StreetStage::Outcome::Quit:
    return EngineStateEnum::HighScore;
  case street::StreetStage::Outcome::LevelFinished:
    return EngineStateEnum::Level3Boss;
  case street::StreetStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::StreetStage &Level3State::stage() const { return m_stage; }

} // namespace openfranko::src::engine::states::level3
