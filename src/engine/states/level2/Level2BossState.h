#ifndef ENGINE_STATES_LEVEL2_LEVEL2BOSSSTATE_H_
#define ENGINE_STATES_LEVEL2_LEVEL2BOSSSTATE_H_

#include "../../street/scenes/BossStage.h"
#include "../shared/StageState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level2 {

class Level2BossState : public shared::StageState<street::scenes::BossStage,
                                                  EngineStateId::Level2Car> {
public:
  using StageState::StageState;
};

} // namespace level2
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL2_LEVEL2BOSSSTATE_H_
