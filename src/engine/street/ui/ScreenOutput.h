#ifndef ENGINE_STREET_UI_SCREENOUTPUT_H_
#define ENGINE_STREET_UI_SCREENOUTPUT_H_

#include "../../../systems/graphics/Display.h"
#include "../../effects/color/AmigaPalette.h"
#include "../core/IndexedSurface.h"

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace ui {

systems::graphics::Display
screenOutput(const core::IndexedSurface &screen, bool shown,
             const effects::color::AmigaPalette &palette,
             effects::color::AmigaColor border);

} // namespace ui
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_UI_SCREENOUTPUT_H_
