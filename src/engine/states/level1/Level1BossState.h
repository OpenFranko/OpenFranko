#ifndef ENGINE_STATES_LEVEL1_LEVEL1BOSSSTATE_H_
#define ENGINE_STATES_LEVEL1_LEVEL1BOSSSTATE_H_

#include "../../street/scenes/BossStage.h"
#include "../shared/StageState.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level1 {

class Level1BossState : public shared::StageState<street::scenes::BossStage,
                                                  EngineStateId::Level1Car> {
public:
  using StageState::StageState;
};

} // namespace level1
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_LEVEL1_LEVEL1BOSSSTATE_H_
