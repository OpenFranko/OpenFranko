#include "Console.h"

#include "ConsoleFont.h"
#include "Hardware.h"
#include "ObjectList.h"
#include "Runtime.h"
#include "Video.h"

#include <algorithm>
#include <cstring>

namespace openfranko::src::systems::jaguar::console {
namespace {

constexpr std::size_t LOG_CAPACITY = 4096;
constexpr int WIDTH = 320;
constexpr int HEIGHT = ROWS * CELL_HEIGHT;
constexpr int ROW_BYTES = WIDTH / 8;
constexpr int ROW_PHRASES = ROW_BYTES / PHRASE_BYTES;
constexpr std::size_t LIST_PHRASES = 8;
constexpr std::size_t BITMAP_PHRASE = 2;
constexpr std::size_t STOP_PHRASE = 4;
constexpr uint16_t TEXT_COLOR = 0xFFF;
constexpr uint16_t FATAL_COLOR = 0x600;
constexpr uint16_t INFO_COLOR = 0x026;

char logText[LOG_CAPACITY];
std::size_t logSize = 0;
alignas(PHRASE_BYTES) uint8_t bitmap[ROW_BYTES * HEIGHT];
alignas(SCALED_ALIGNMENT) uint64_t live[LIST_PHRASES];
uint64_t pattern[LIST_PHRASES];
uint16_t paper = 0;
uint16_t ink = 0;

void drawCharacter(int column, int row, unsigned char character) {
  const uint8_t *rows = glyph(character);
  if (!rows || column < 0 || column >= COLUMNS || row < 0 || row >= ROWS) {
    return;
  }
  for (int line = 0; line < GLYPH_HEIGHT; ++line) {
    const int y = row * CELL_HEIGHT + line;
    for (int bit = 0; bit < GLYPH_WIDTH; ++bit) {
      if (rows[line] & (0x10 >> bit)) {
        const int x = column * CELL_WIDTH + bit;
        bitmap[y * ROW_BYTES + x / 8] |= static_cast<uint8_t>(0x80 >> (x & 7));
      }
    }
  }
}

int drawText(int row, const char *text, std::size_t size) {
  int column = 0;
  for (std::size_t i = 0; i < size && row < ROWS; ++i) {
    const unsigned char character = static_cast<unsigned char>(text[i]);
    if (character == '\n' || column == COLUMNS) {
      ++row;
      column = 0;
      if (character == '\n') {
        continue;
      }
    }
    drawCharacter(column++, row, character);
  }
  return row + 1;
}

std::size_t tailStart(int rows) {
  std::size_t start = logSize;
  int lines = 0;
  while (start > 0 && lines < rows) {
    --start;
    if (logText[start] == '\n' && start + 1 < logSize) {
      ++lines;
      if (lines == rows) {
        ++start;
        break;
      }
    }
  }
  return start;
}

void refresh() {
  std::memcpy(live, pattern, sizeof(live));
  word(CLUT) = paper;
  word(CLUT + 2) = ink;
  word(BG) = paper;
}

void render(const std::string &title) {
  std::fill(std::begin(bitmap), std::end(bitmap), 0);
  const int row = drawText(0, title.data(), title.size()) + 1;
  const std::size_t start = tailStart(ROWS - row);
  drawText(row, logText + start, logSize - start);
}

void prepare(const Geometry &geometry, bool fatal) {
  setupVideo(geometry);
  paper = toRgb16(fatal ? FATAL_COLOR : INFO_COLOR);
  ink = toRgb16(TEXT_COLOR);
  const uint32_t base = reinterpret_cast<uint32_t>(live);
  const uint32_t stop = base + STOP_PHRASE * PHRASE_BYTES;
  pattern[0] = branchPhrase(geometry.lastHalfLine, Branch::Below, stop);
  pattern[1] = branchPhrase(geometry.firstHalfLine, Branch::Above, stop);
  BitmapObject text;
  text.data = reinterpret_cast<uint32_t>(bitmap);
  text.depth = Depth::Bits1;
  text.x = std::max(0, (geometry.columns - WIDTH) / 2);
  text.y = (geometry.firstHalfLine + std::max(0, geometry.rows - HEIGHT)) & ~1;
  text.height = HEIGHT;
  text.dataWidth = ROW_PHRASES;
  text.imageWidth = ROW_PHRASES;
  bitmapPhrases(text, stop, pattern + BITMAP_PHRASE);
  pattern[STOP_PHRASE] = stopPhrase();
}

bool attached = false;
std::string attachedTitle;

} // namespace

void clear() {
  logSize = 0;
  if (attached) {
    render(attachedTitle);
  }
}

void write(const char *text, std::size_t size) {
  if (size >= LOG_CAPACITY) {
    text += size - LOG_CAPACITY + 1;
    size = LOG_CAPACITY - 1;
  }
  if (logSize + size >= LOG_CAPACITY) {
    const std::size_t drop = logSize + size - (LOG_CAPACITY - 1);
    std::memmove(logText, logText + drop, logSize - drop);
    logSize -= drop;
  }
  std::memcpy(logText + logSize, text, size);
  logSize += size;
}

void print(const std::string &text) {
  write(text.data(), text.size());
  write("\n", 1);
  if (attached) {
    render(attachedTitle);
  }
}

void attach(const std::string &title) {
  const Geometry geometry = detectGeometry();
  attachedTitle = title;
  render(title);
  prepare(geometry, false);
  waitBlanking(geometry);
  refresh();
  setOlp(reinterpret_cast<uint32_t>(live));
  runtime::setVideoHandler(refresh);
  runtime::enableVideoInterrupt(geometry.lastHalfLine);
  attached = true;
}

[[noreturn]] void show(const std::string &title, bool fatal) {
  asm volatile("move.w #0x2700,%%sr" ::: "cc");
  word(INT1) = 0x1F00;
  attached = false;
  const Geometry geometry = detectGeometry();
  render(title);
  prepare(geometry, fatal);
  waitBlanking(geometry);
  refresh();
  setOlp(reinterpret_cast<uint32_t>(live));
  for (;;) {
    waitDisplay(geometry);
    waitBlanking(geometry);
    refresh();
  }
}

} // namespace openfranko::src::systems::jaguar::console
