#include "StreetVideo.h"

#include "../../street/ui/StageFrame.h"

namespace openfranko::src::engine::states::shared {

void showStageFrame(systems::graphics::VideoSystem &videoSystem,
                    const systems::graphics::Display &frame,
                    const GameOptions &options) {
  videoSystem.setNtsc(street::ui::stageLayout(options).ntsc);
  videoSystem.show(frame);
}

} // namespace openfranko::src::engine::states::shared
