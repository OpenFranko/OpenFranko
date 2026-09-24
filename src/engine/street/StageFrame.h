#ifndef ENGINE_STREET_STAGEFRAME_H_
#define ENGINE_STREET_STAGEFRAME_H_

#include "../amal/Machine.h"
#include "../effects/AmigaPalette.h"
#include "IndexedSurface.h"
#include "StatusPanel.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

constexpr int FRAME_WIDTH = 304;
constexpr int FRAME_HEIGHT = 255;
constexpr int DISPLAY_X = 128;
constexpr int DISPLAY_TOP = 47;

uint32_t toArgb(effects::AmigaColor color);
const effects::AmigaPalette &levelPalette(bool mono);
const effects::AmigaPalette &panelPalette();

void composeFrame(std::vector<uint32_t> &frame, const IndexedSurface *display,
                  const effects::AmigaPalette &palette,
                  const amal::Object &screenDisplay, int offsetX,
                  const StatusPanel *panel,
                  const effects::AmigaPalette &panelColors);

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_STAGEFRAME_H_
