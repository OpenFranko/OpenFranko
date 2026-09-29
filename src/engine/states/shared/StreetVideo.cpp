#include "StreetVideo.h"

#include "../../street/ui/StageFrame.h"

namespace openfranko::src::engine::states::shared {

void showStageFrame(systems::graphics::Monitor &monitor,
                    const systems::graphics::Display &frame,
                    const GameOptions &options) {
  monitor.setNtsc(street::ui::stageLayout(options).ntsc);
  monitor.show(frame);
}

void showSceneFrame(systems::graphics::Monitor &monitor,
                    systems::graphics::Display frame, const VisibleRows &rows) {
  systems::graphics::cropRows(frame, rows.first, rows.count);
  monitor.show(frame);
}

} // namespace openfranko::src::engine::states::shared
