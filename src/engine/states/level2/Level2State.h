#ifndef ENGINE_STATES_LEVEL2_LEVEL2STATE_H_
#define ENGINE_STATES_LEVEL2_LEVEL2STATE_H_

#include "../../street/scenes/StreetStage.h"
#include "../shared/StageState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level2 {

class Level2State : public shared::StageState<street::scenes::StreetStage,
                                              EngineStateId::Level2Boss> {
public:
  using StageState::StageState;
};

} // namespace level2
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL2_LEVEL2STATE_H_
