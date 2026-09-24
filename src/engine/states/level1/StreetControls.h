#ifndef ENGINE_STATES_STREETCONTROLS_H_
#define ENGINE_STATES_STREETCONTROLS_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../effects/GameOptions.h"
#include "../../street/StreetStage.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level1 {

street::StreetInput
readStreetInput(const systems::ControllerSystem &controller);
void restartMenuMusic(systems::AudioSystem &audio,
                      const effects::GameOptions &options);

} // namespace level1
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_STREETCONTROLS_H_
