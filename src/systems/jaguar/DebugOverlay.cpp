#include "DebugOverlay.h"

#include "TextPanel.h"

namespace openfranko::src::systems::jaguar::overlay {
namespace {

alignas(8) uint16_t buffer[WIDTH * HEIGHT];
bool enabled = false;

} // namespace

void setEnabled(bool on) { enabled = on; }

bool isEnabled() { return enabled; }

void setLine(int line, const char *text) {
  if (line >= 0 && line < LINES) {
    drawPanelLine(buffer, WIDTH, line, text);
  }
}

const uint16_t *pixels() { return buffer; }

} // namespace openfranko::src::systems::jaguar::overlay
