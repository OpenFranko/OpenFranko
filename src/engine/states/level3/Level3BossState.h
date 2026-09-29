#ifndef ENGINE_STATES_LEVEL3_LEVEL3BOSSSTATE_H_
#define ENGINE_STATES_LEVEL3_LEVEL3BOSSSTATE_H_

#include "../../street/scenes/BossStage.h"
#include "../shared/StageState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level3 {

class Level3BossState : public shared::StageState<street::scenes::BossStage,
                                                  EngineStateId::Ending> {
public:
  using StageState::StageState;
};

} // namespace level3
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL3_LEVEL3BOSSSTATE_H_
