#ifndef ENGINE_STATES_STREETCONTROLS_H_
#define ENGINE_STATES_STREETCONTROLS_H_

#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/GameOptions.h"
#include "../../street/StreetStage.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level1 {

street::StreetInput
readStreetInput(const systems::ControllerSystem &controller);
void showStageFrame(systems::VideoSystem &videoSystem,
                    const std::vector<uint32_t> &frame,
                    const effects::GameOptions &options);

} // namespace level1
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_STREETCONTROLS_H_
