#ifndef ENGINE_STATES_SHARED_STREETVIDEO_H_
#define ENGINE_STATES_SHARED_STREETVIDEO_H_

#include "../../../systems/graphics/VideoSystem.h"
#include "../../AmigaDisplay.h"
#include "../../GameOptions.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

void showStageFrame(systems::graphics::VideoSystem &videoSystem,
                    const systems::graphics::Display &frame,
                    const GameOptions &options);
void showSceneFrame(systems::graphics::VideoSystem &videoSystem,
                    systems::graphics::Display frame, const VisibleRows &rows);

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SHARED_STREETVIDEO_H_
