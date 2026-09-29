#include "Level2State.h"

#include "../shared/StreetControls.h"
#include "../shared/StreetVideo.h"

namespace openfranko::src::engine::states::level2 {

Level2State::Level2State(systems::graphics::VideoSystem &videoSystem,
                         systems::audio::AudioSystem &audioSystem,
                         systems::input::ControllerSystem &controllerSystem,
                         GameOptions &options,
                         street::session::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem, session.version),
      m_stage(m_host, session, options) {}

std::optional<EngineStateEnum> Level2State::update() {
  m_stage.advance(
      shared::readStreetInput(m_controllerSystem, m_host.version()));
  shared::showStageFrame(m_videoSystem, m_stage.output(), m_options);

  switch (m_stage.outcome()) {
  case street::scenes::StreetStage::Outcome::GameOver:
    return EngineStateEnum::GameOver;
  case street::scenes::StreetStage::Outcome::Quit:
    return EngineStateEnum::HighScore;
  case street::scenes::StreetStage::Outcome::LevelFinished:
    return EngineStateEnum::Level2Boss;
  case street::scenes::StreetStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::scenes::StreetStage &Level2State::stage() const {
  return m_stage;
}

} // namespace openfranko::src::engine::states::level2
