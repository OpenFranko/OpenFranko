#include "ScreenOutput.h"

#include <utility>

namespace openfranko::src::engine::street::ui {

systems::graphics::Display
screenOutput(const core::IndexedSurface &screen, bool shown,
             const effects::color::AmigaPalette &palette,
             effects::color::AmigaColor border) {
  systems::graphics::Display display;
  display.width = screen.width();
  display.height = screen.height();
  display.displayHeight = screen.height();
  display.border = border;
  if (!shown) {
    return display;
  }
  systems::graphics::Layer layer;
  layer.pixels = screen.pixels().data();
  layer.stride = screen.width();
  layer.sourceColumns = screen.width();
  layer.sourceRows = screen.height();
  layer.columns = screen.width();
  layer.rows = screen.height();
  layer.mask = static_cast<uint8_t>(palette.size() - 1);
  layer.palette = palette;
  display.layers.push_back(std::move(layer));
  return display;
}

} // namespace openfranko::src::engine::street::ui
