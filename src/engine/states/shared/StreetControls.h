#ifndef ENGINE_STATES_STREETCONTROLS_H_
#define ENGINE_STATES_STREETCONTROLS_H_

#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../GameVersion.h"
#include "../../effects/GameOptions.h"
#include "../../street/StreetStage.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

street::StreetInput readStreetInput(const systems::ControllerSystem &controller,
                                    GameVersion version);
void showStageFrame(systems::VideoSystem &videoSystem,
                    const systems::Display &frame,
                    const effects::GameOptions &options);

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_STREETCONTROLS_H_
