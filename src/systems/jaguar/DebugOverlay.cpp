#include "DebugOverlay.h"

#include "ConsoleFont.h"
#include "TextPanel.h"

namespace openfranko::src::systems::jaguar::overlay {
namespace {

constexpr int COLUMNS = WIDTH / CELL_WIDTH;

alignas(8) uint16_t buffer[WIDTH * HEIGHT];
char shown[LINES][COLUMNS] = {};
bool enabled = false;

} // namespace

void setEnabled(bool on) { enabled = on; }

bool isEnabled() { return enabled; }

void setLine(int line, const char *text) {
  if (line < 0 || line >= LINES) {
    return;
  }
  bool ended = false;
  for (int column = 0; column < COLUMNS; ++column) {
    ended = ended || text[column] == '\0';
    const char wanted = ended ? ' ' : text[column];
    char &current = shown[line][column];
    if (current != wanted) {
      drawPanelCell(buffer, WIDTH, line, column, wanted);
      current = wanted;
    }
  }
}

const uint16_t *pixels() { return buffer; }

} // namespace openfranko::src::systems::jaguar::overlay
