#ifndef ENGINE_STATES_SHARED_STREETCONTROLS_H_
#define ENGINE_STATES_SHARED_STREETCONTROLS_H_

#include "../../../systems/input/ControllerSystem.h"
#include "../../GameVersion.h"
#include "../../street/scenes/StreetHost.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

street::scenes::StreetInput
readStreetInput(const systems::input::ControllerSystem &controller,
                GameVersion version);

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SHARED_STREETCONTROLS_H_
