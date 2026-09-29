#ifndef ENGINE_STATES_SHARED_STAGESTATE_H_
#define ENGINE_STATES_SHARED_STAGESTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../GameOptions.h"
#include "../../street/session/GameSession.h"
#include "../EngineState.h"
#include "EngineStreetHost.h"
#include "StreetControls.h"
#include "StreetVideo.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

template <typename Stage, EngineStateId NEXT>
class StageState : public EngineState {
public:
  StageState(systems::graphics::VideoSystem &videoSystem,
             systems::audio::AudioSystem &audioSystem,
             systems::input::ControllerSystem &controllerSystem,
             GameOptions &options, street::session::GameSession &session);

  std::optional<EngineStateId> update() override;

  const Stage &stage() const;

private:
  systems::graphics::VideoSystem &m_videoSystem;
  systems::input::ControllerSystem &m_controllerSystem;
  const GameOptions &m_options;
  EngineStreetHost m_host;
  Stage m_stage;
};

template <typename Stage, EngineStateId NEXT>
StageState<Stage, NEXT>::StageState(
    systems::graphics::VideoSystem &videoSystem,
    systems::audio::AudioSystem &audioSystem,
    systems::input::ControllerSystem &controllerSystem, GameOptions &options,
    street::session::GameSession &session)
    : m_videoSystem(videoSystem), m_controllerSystem(controllerSystem),
      m_options(options), m_host(audioSystem, session.version),
      m_stage(m_host, session, options) {}

template <typename Stage, EngineStateId NEXT>
std::optional<EngineStateId> StageState<Stage, NEXT>::update() {
  m_stage.advance(readStreetInput(m_controllerSystem, m_host.version()));
  showStageFrame(m_videoSystem, m_stage.output(), m_options);

  switch (m_stage.outcome()) {
  case Stage::Outcome::GameOver:
    return EngineStateId::GameOver;
  case Stage::Outcome::Quit:
    return EngineStateId::HighScore;
  case Stage::Outcome::Cleared:
    return NEXT;
  case Stage::Outcome::Playing:
    break;
  }
  return std::nullopt;
}

template <typename Stage, EngineStateId NEXT>
const Stage &StageState<Stage, NEXT>::stage() const {
  return m_stage;
}

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SHARED_STAGESTATE_H_
