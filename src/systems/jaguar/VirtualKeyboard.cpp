#include "VirtualKeyboard.h"

#include "TextPanel.h"

#include <cstring>

namespace openfranko::src::systems::jaguar::keyboard {
namespace {

constexpr char LETTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
constexpr int COUNT = sizeof(LETTERS) - 1;
constexpr int SHOWN = 4;
constexpr const char *HELP = "A TYPE  B DELETE  C ENTER  0 HIDE";

alignas(8) uint16_t buffer[WIDTH * HEIGHT];
bool open = false;
int current = 0;

char shown(int index) {
  const char letter = LETTERS[((index % COUNT) + COUNT) % COUNT];
  return letter == ' ' ? '_' : letter;
}

void redraw() {
  char line[40];
  std::memset(line, ' ', sizeof(line));
  int at = 0;
  for (int offset = -SHOWN; offset <= SHOWN; ++offset) {
    if (offset == 0) {
      line[at] = '[';
      line[at + 1] = shown(current);
      line[at + 2] = ']';
    } else {
      line[at + 1] = shown(current + offset);
    }
    at += 4;
  }
  line[at] = '\0';
  drawPanelLine(buffer, WIDTH, 0, line);
  drawPanelLine(buffer, WIDTH, 1, HELP);
}

} // namespace

void setOpen(bool state) {
  if (state && !open) {
    current = 0;
    redraw();
  }
  open = state;
}

bool isOpen() { return open; }

void move(int steps) {
  current = ((current + steps) % COUNT + COUNT) % COUNT;
  redraw();
}

char selected() { return LETTERS[current]; }

const uint16_t *pixels() { return buffer; }

} // namespace openfranko::src::systems::jaguar::keyboard
