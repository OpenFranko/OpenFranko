#ifndef ENGINE_STATES_SHARED_STAGESTATE_H_
#define ENGINE_STATES_SHARED_STAGESTATE_H_

#include "../../../systems/graphics/Monitor.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../GameOptions.h"
#include "../../GameVersion.h"
#include "../../street/scenes/StreetHost.h"
#include "../../street/session/GameSession.h"
#include "../EngineState.h"
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
  StageState(systems::graphics::Monitor &monitor,
             street::scenes::StreetHost &host,
             systems::input::ControllerSystem &controllerSystem,
             GameOptions &options, street::session::GameSession &session);

  std::optional<EngineStateId> update() override;

  const Stage &stage() const;

private:
  systems::graphics::Monitor &m_monitor;
  systems::input::ControllerSystem &m_controllerSystem;
  const GameOptions &m_options;
  GameVersion m_version;
  Stage m_stage;
};

template <typename Stage, EngineStateId NEXT>
StageState<Stage, NEXT>::StageState(
    systems::graphics::Monitor &monitor, street::scenes::StreetHost &host,
    systems::input::ControllerSystem &controllerSystem, GameOptions &options,
    street::session::GameSession &session)
    : m_monitor(monitor), m_controllerSystem(controllerSystem),
      m_options(options), m_version(session.version),
      m_stage(host, session, options) {}

template <typename Stage, EngineStateId NEXT>
std::optional<EngineStateId> StageState<Stage, NEXT>::update() {
  m_stage.advance(readStreetInput(m_controllerSystem, m_version));
  showStageFrame(m_monitor, m_stage.output(), m_options);

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
