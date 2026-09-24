#ifndef ENGINE_STATES_CONTINUESTATE_H_
#define ENGINE_STATES_CONTINUESTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../street/ContinueScene.h"
#include "../../street/GameSession.h"
#include "../IEngineState.h"
#include "../level1/EngineStreetHost.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace continueSelect {

class ContinueState : public IEngineState {
public:
  ContinueState(systems::VideoSystem &videoSystem,
                systems::AudioSystem &audioSystem,
                systems::ControllerSystem &controllerSystem,
                street::GameSession &session);
  ~ContinueState();

  std::optional<EngineStateEnum> update() override;

  const street::ContinueScene &scene() const;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  level1::EngineStreetHost m_host;
  street::ContinueScene m_scene;
  std::vector<uint32_t> m_frame;
};

} // namespace continueSelect
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_CONTINUESTATE_H_
