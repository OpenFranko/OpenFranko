#include "VirtualKeyboard.h"

#include "TextPanel.h"

#include <cstring>

namespace openfranko::src::systems::jaguar::keyboard {
namespace {

constexpr char LETTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
constexpr int COUNT = sizeof(LETTERS) - 1;
constexpr int SHOWN = 4;
constexpr int COMPACT_SHOWN = 2;
constexpr int CELL_CHARACTERS = 4;
constexpr const char *HELP = "A TYPE  B DELETE  C ENTER  0 HIDE";
constexpr const char *COMPACT_HELP = "A TYPE  0 HIDE";

alignas(8) uint16_t buffer[WIDTH * HEIGHT];
bool open = false;
bool compact = false;
int current = 0;

char shown(int index) {
  const char letter = LETTERS[((index % COUNT) + COUNT) % COUNT];
  return letter == ' ' ? '_' : letter;
}

void redraw() {
  char line[40];
  std::memset(line, ' ', sizeof(line));
  const int shownEachSide = compact ? COMPACT_SHOWN : SHOWN;
  int at = 0;
  for (int offset = -shownEachSide; offset <= shownEachSide; ++offset) {
    if (offset == 0) {
      line[at] = '[';
      line[at + 1] = shown(current);
      line[at + 2] = ']';
    } else {
      line[at + 1] = shown(current + offset);
    }
    at += CELL_CHARACTERS;
  }
  line[at] = '\0';
  drawPanelLine(buffer, width(), 0, line);
  drawPanelLine(buffer, width(), 1, compact ? COMPACT_HELP : HELP);
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

void setCompact(bool state) {
  if (state != compact) {
    compact = state;
    redraw();
  }
}

bool isCompact() { return compact; }

int width() { return compact ? COMPACT_WIDTH : WIDTH; }

void move(int steps) {
  current = ((current + steps) % COUNT + COUNT) % COUNT;
  redraw();
}

void select(char letter) {
  for (int index = 0; index < COUNT; ++index) {
    if (LETTERS[index] == letter) {
      current = index;
      redraw();
      return;
    }
  }
}

char selected() { return LETTERS[current]; }

const uint16_t *pixels() { return buffer; }

} // namespace openfranko::src::systems::jaguar::keyboard
