#ifndef ENGINE_STATES_LEVEL2_LEVEL2CARSTATE_H_
#define ENGINE_STATES_LEVEL2_LEVEL2CARSTATE_H_

#include "../../street/scenes/CarStage.h"
#include "../shared/StageState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level2 {

class Level2CarState
    : public shared::StageState<street::scenes::CarStage,
                                EngineStateId::StageProtectionCheck> {
public:
  using StageState::StageState;
};

} // namespace level2
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL2_LEVEL2CARSTATE_H_
