#ifndef ENGINE_STATES_WORLDSOFTWARESTATE_H_
#define ENGINE_STATES_WORLDSOFTWARESTATE_H_

#include "../../../systems/VideoSystem.h"
#include "../IEngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace worldSoftware {

class WorldSoftwareState : public IEngineState {
public:
  WorldSoftwareState(systems::VideoSystem &videoSystem);
  ~WorldSoftwareState();

  std::optional<EngineStateEnum> update() override;

private:
  systems::VideoSystem m_videoSystem;
};

} // namespace worldSoftware
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_WORLDSOFTWARESTATE_H_