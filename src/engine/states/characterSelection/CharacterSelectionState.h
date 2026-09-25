#ifndef ENGINE_STATES_CHARACTERSELECTIONSTATE_H_
#define ENGINE_STATES_CHARACTERSELECTIONSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/AmigaDisplay.h"
#include "../../effects/CharacterSelection.h"
#include "../../effects/GameOptions.h"
#include "../../street/GameSession.h"
#include "../IEngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace characterSelection {

class CharacterSelectionState : public IEngineState {
public:
  CharacterSelectionState(systems::VideoSystem &videoSystem,
                          systems::AudioSystem &audioSystem,
                          systems::ControllerSystem &controllerSystem,
                          effects::GameOptions &options,
                          street::GameSession &session);
  ~CharacterSelectionState();

  std::optional<EngineStateEnum> update() override;

private:
  void draw();
  EngineStateEnum firstStreet() const;

  systems::VideoSystem &m_videoSystem;
  systems::AudioSystem &m_audioSystem;
  systems::ControllerSystem &m_controllerSystem;
  street::GameSession &m_session;
  effects::CharacterSelection m_selection;
  effects::VisibleRows m_rows;
};

} // namespace characterSelection
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_CHARACTERSELECTIONSTATE_H_
