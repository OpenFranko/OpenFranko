#ifndef ENGINE_STATES_LEVEL3_LEVEL3STATE_H_
#define ENGINE_STATES_LEVEL3_LEVEL3STATE_H_

#include "../../street/scenes/StreetStage.h"
#include "../shared/StageState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level3 {

class Level3State : public shared::StageState<street::scenes::StreetStage,
                                              EngineStateId::Level3Boss> {
public:
  using StageState::StageState;
};

} // namespace level3
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL3_LEVEL3STATE_H_
