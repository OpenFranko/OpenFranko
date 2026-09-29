#ifndef ENGINE_STATES_SHARED_STREETCONTROLS_H_
#define ENGINE_STATES_SHARED_STREETCONTROLS_H_

#include "../../../systems/graphics/VideoSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../GameVersion.h"
#include "../../effects/core/GameOptions.h"
#include "../../street/scenes/StreetStage.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

street::scenes::StreetInput
readStreetInput(const systems::input::ControllerSystem &controller,
                GameVersion version);
void showStageFrame(systems::graphics::VideoSystem &videoSystem,
                    const systems::graphics::Display &frame,
                    const effects::core::GameOptions &options);

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SHARED_STREETCONTROLS_H_
