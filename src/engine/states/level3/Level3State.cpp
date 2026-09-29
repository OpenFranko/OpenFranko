#include "Level3State.h"

#include "../shared/StreetControls.h"
#include "../shared/StreetVideo.h"

namespace openfranko::src::engine::states::level3 {

Level3State::Level3State(systems::graphics::VideoSystem &videoSystem,
                         systems::audio::AudioSystem &audioSystem,
                         systems::input::ControllerSystem &controllerSystem,
                         GameOptions &options,
                         street::session::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem, session.version),
      m_stage(m_host, session, options) {}

std::optional<EngineStateId> Level3State::update() {
  m_stage.advance(
      shared::readStreetInput(m_controllerSystem, m_host.version()));
  shared::showStageFrame(m_videoSystem, m_stage.output(), m_options);

  switch (m_stage.outcome()) {
  case street::scenes::StreetStage::Outcome::GameOver:
    return EngineStateId::GameOver;
  case street::scenes::StreetStage::Outcome::Quit:
    return EngineStateId::HighScore;
  case street::scenes::StreetStage::Outcome::LevelFinished:
    return EngineStateId::Level3Boss;
  case street::scenes::StreetStage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

const street::scenes::StreetStage &Level3State::stage() const {
  return m_stage;
}

} // namespace openfranko::src::engine::states::level3
