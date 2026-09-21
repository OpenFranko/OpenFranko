#ifndef ENGINE_STATES_MIRAGESTATE_H_
#define ENGINE_STATES_MIRAGESTATE_H_

#include "../../systems/ControllerSystem.h"
#include "../../systems/VideoSystem.h"
#include "../IEngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {

class MirageState : public IEngineState {
public:
  MirageState(systems::VideoSystem &videoSystem,
              systems::ControllerSystem &controllerSystem);
  ~MirageState();

  std::optional<EngineStateEnum> update() override;

private:
  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
};

} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_MIRAGESTATE_H_