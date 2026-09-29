#ifndef ENGINE_STATES_LEVEL1_LEVEL1CARSTATE_H_
#define ENGINE_STATES_LEVEL1_LEVEL1CARSTATE_H_

#include "../../street/scenes/CarStage.h"
#include "../shared/StageState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level1 {

class Level1CarState : public shared::StageState<street::scenes::CarStage,
                                                 EngineStateId::Level2> {
public:
  using StageState::StageState;
};

} // namespace level1
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL1_LEVEL1CARSTATE_H_
