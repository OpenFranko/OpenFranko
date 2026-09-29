#ifndef ENGINE_STATES_LEVEL1_LEVEL1STATE_H_
#define ENGINE_STATES_LEVEL1_LEVEL1STATE_H_

#include "../../street/scenes/StreetStage.h"
#include "../shared/StageState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level1 {

class Level1State : public shared::StageState<street::scenes::StreetStage,
                                              EngineStateId::Level1Boss> {
public:
  using StageState::StageState;
};

} // namespace level1
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL1_LEVEL1STATE_H_
