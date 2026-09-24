#ifndef ENGINE_STATES_MIRAGESTATE_H_
#define ENGINE_STATES_MIRAGESTATE_H_

#include "../../../systems/VideoSystem.h"
#include "../../effects/FotoSequence.h"
#include "../IEngineState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace mirage {

class MirageState : public IEngineState {
public:
  explicit MirageState(systems::VideoSystem &videoSystem);
  ~MirageState();

  std::optional<EngineStateEnum> update() override;

private:
  systems::VideoSystem &m_videoSystem;
  effects::FotoSequence m_sequence;
};

} // namespace mirage
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_MIRAGESTATE_H_
