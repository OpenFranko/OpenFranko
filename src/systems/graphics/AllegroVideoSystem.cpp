#include "graphics/VideoSystem.h"

#include "graphics/IndexedRasterizer.h"

#include <algorithm>
#include <allegro.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <dos.h>
#include <pc.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace openfranko::src::systems::graphics {
namespace {

constexpr int SCREEN_WIDTH = 376;
constexpr int SCREEN_HEIGHT = 282;
constexpr int PLANES = 4;
constexpr int SEQUENCER_PORT = 0x3C4;
constexpr int GRAPHICS_PORT = 0x3CE;
constexpr int MAP_MASK = 0x02;
constexpr int MODE_REGISTER = 0x05;
constexpr int WRITE_MODES = 0x03;
constexpr int LATCH_WRITE = 0x01;
constexpr int ALL_PLANES = 0x0F;
constexpr int PLANE_SHIFT = 8;
constexpr int DAC_SHIFT = 2;
constexpr std::size_t BAND_ROWS = 8;
constexpr int SCREEN_HERTZ = PAL_HERTZ;
constexpr int STATUS_PORT = 0x3DA;
constexpr int DISPLAY_OFF = 0x01;
constexpr int VERTICAL_RETRACE = 0x08;
constexpr int TIMER_COMMAND_PORT = 0x43;
constexpr int CLOCK_PORT = 0x42;
constexpr int SPEAKER_PORT = 0x61;
constexpr int CLOCK_GATE = 0x01;
constexpr int SPEAKER_DATA = 0x02;
constexpr int CLOCK_MODE = 0xB4;
constexpr int CLOCK_LATCH = 0x80;
constexpr int CLOCK_WRAP = 0x10000;
constexpr int BYTE_BITS = 8;
constexpr int CLOCK_CHECK_READS = 100;
constexpr int VERTICAL_BLANK_TICKS = 24;
constexpr int RETRACE_TIMEOUT = 3;
constexpr int PHASE_TIMEOUT = SCREEN_HERTZ;
constexpr int QUICK_PART = 8;
constexpr int PERIOD_SMOOTHING = 8;
constexpr int PERIOD_SAMPLES = 64;
constexpr int FASTEST_HERTZ = 55;
constexpr int SLOWEST_HERTZ = 45;
constexpr int VIRTUAL_WIDTH = 896;
constexpr int CRTC_PORT = 0x3D4;
constexpr int START_HIGH = 0x0C;
constexpr int START_LOW = 0x0D;
constexpr int ATTRIBUTE_PORT = 0x3C0;
constexpr int PEL_PANNING = 0x13;
constexpr int PALETTE_ADDRESS_SOURCE = 0x20;
constexpr int PEL_STEP = 2;
constexpr int NO_REBASE = -1;

volatile int vbls = 0;

void countVbl() { ++vbls; }
END_OF_FUNCTION(countVbl)

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Video system error: " + cause + ": " +
                           allegro_error);
}

struct RowWork {
  int row = 0;
  int from = NO_ROW;
  int shift = 0;
  std::array<Span, 2> spans{};
};

int panOf(const IndexedFrame &frame, int height) {
  int pan = 0;
  for (int row = 0; row < height; ++row) {
    const RowChange &change = frame.changes[static_cast<std::size_t>(row)];
    if (change.from != NO_ROW || change.shift == 0 ||
        (pan != 0 && change.shift != pan)) {
      return 0;
    }
    pan = change.shift;
  }
  return pan;
}

bool movesRows(const IndexedFrame &frame) {
  return std::any_of(frame.changes.begin(), frame.changes.end(),
                     [](const RowChange &change) {
                       return change.from != NO_ROW || change.shift != 0;
                     });
}

struct Placement {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  int step = 1;
  int sourceWidth = 0;
  int sourceHeight = 0;
};

bool operator!=(const Placement &left, const Placement &right) {
  return left.x != right.x || left.y != right.y || left.width != right.width ||
         left.height != right.height || left.step != right.step ||
         left.sourceWidth != right.sourceWidth ||
         left.sourceHeight != right.sourceHeight;
}

Placement place(const Display &display) {
  Placement placement;
  if (display.width <= 0 || display.height <= 0) {
    return placement;
  }
  placement.step = (display.width + SCREEN_WIDTH - 1) / SCREEN_WIDTH;
  placement.width = display.width / placement.step;
  placement.height = std::clamp(
      std::max(display.displayHeight, 1) / placement.step, 1, SCREEN_HEIGHT);
  placement.x = (SCREEN_WIDTH - placement.width) / 2 / PLANES * PLANES;
  placement.y = (SCREEN_HEIGHT - placement.height) / 2;
  placement.sourceWidth = display.width;
  placement.sourceHeight = display.height;
  return placement;
}

int sourceRow(const Placement &placement, int row) {
  return placement.height == placement.sourceHeight
             ? row
             : row * placement.sourceHeight / placement.height;
}

int brightness(uint16_t color) {
  return (color >> 8 & 0xF) + (color >> 4 & 0xF) + (color & 0xF);
}

int darkestSlot(const std::array<uint16_t, FRAME_COLORS> &palette) {
  const auto darkest = std::min_element(
      palette.begin(), palette.end(), [](uint16_t left, uint16_t right) {
        return brightness(left) < brightness(right);
      });
  return static_cast<int>(darkest - palette.begin());
}

int dacLevel(uint16_t color, int shift) {
  const int level = color >> shift & 0xF;
  return level << DAC_SHIFT | level >> DAC_SHIFT;
}

std::size_t firstDifference(const uint8_t *left, const uint8_t *right,
                            std::size_t words) {
  std::size_t remaining = words;
  bool same = true;
  asm volatile("cld\n\trepe cmpsl"
               : "+S"(left), "+D"(right), "+c"(remaining), "=@ccz"(same)
               :
               : "memory");
  return same ? words : words - remaining - 1;
}

std::size_t lastDifference(const uint8_t *left, const uint8_t *right,
                           std::size_t words) {
  std::size_t remaining = words;
  bool same = true;
  const uint8_t *leftWord = left + (words - 1) * sizeof(uint32_t);
  const uint8_t *rightWord = right + (words - 1) * sizeof(uint32_t);
  asm volatile("std\n\trepe cmpsl\n\tcld"
               : "+S"(leftWord), "+D"(rightWord), "+c"(remaining), "=@ccz"(same)
               :
               : "memory");
  return same ? words : words - remaining - 1;
}

std::size_t leadingWords(const uint8_t *left, const uint8_t *right,
                         std::size_t count) {
  const std::size_t words = count / sizeof(uint32_t);
  std::size_t equal =
      words > 0 ? firstDifference(left, right, words) * sizeof(uint32_t) : 0;
  while (equal < count && left[equal] == right[equal]) {
    ++equal;
  }
  return equal;
}

std::size_t trailingWords(const uint8_t *left, const uint8_t *right,
                          std::size_t count) {
  std::size_t equal = 0;
  while (count % sizeof(uint32_t) != 0) {
    if (left[count - 1] != right[count - 1]) {
      return equal;
    }
    --count;
    ++equal;
  }
  const std::size_t words = count / sizeof(uint32_t);
  const std::size_t equalWords =
      words > 0 ? lastDifference(left, right, words) : 0;
  equal += equalWords * sizeof(uint32_t);
  count -= equalWords * sizeof(uint32_t);
  while (count > 0 && left[count - 1] == right[count - 1]) {
    --count;
    ++equal;
  }
  return equal;
}

void copyLatches(uintptr_t from, uintptr_t to, int count, bool backward) {
  if (count <= 0) {
    return;
  }
  const uint16_t selector = static_cast<uint16_t>(screen->seg);
  if (backward) {
    from += static_cast<uintptr_t>(count - 1);
    to += static_cast<uintptr_t>(count - 1);
    asm volatile("pushl %%es\n\t"
                 "movw %w3, %%es\n\t"
                 "std\n\t"
                 "rep movsb %%fs:(%%esi), %%es:(%%edi)\n\t"
                 "cld\n\t"
                 "popl %%es"
                 : "+S"(from), "+D"(to), "+c"(count)
                 : "r"(selector)
                 : "memory");
  } else {
    asm volatile("pushl %%es\n\t"
                 "movw %w3, %%es\n\t"
                 "cld\n\t"
                 "rep movsb %%fs:(%%esi), %%es:(%%edi)\n\t"
                 "popl %%es"
                 : "+S"(from), "+D"(to), "+c"(count)
                 : "r"(selector)
                 : "memory");
  }
}

uint32_t gatherWord(const uint8_t *source, int stride) {
  return static_cast<uint32_t>(source[0]) |
         static_cast<uint32_t>(source[stride]) << 8 |
         static_cast<uint32_t>(source[2 * stride]) << 16 |
         static_cast<uint32_t>(source[3 * stride]) << 24;
}

template <int Stride>
void writePlaneWords(uintptr_t &address, const uint8_t *&source, int words) {
  int pairs = words / 2;
  if (pairs > 0) {
    asm volatile(
        "1:\n\t"
        "movb %c[s3](%%esi), %%ah\n\t"
        "movb %c[s2](%%esi), %%al\n\t"
        "shll $16, %%eax\n\t"
        "movb %c[s1](%%esi), %%ah\n\t"
        "movb (%%esi), %%al\n\t"
        "movl %%eax, %%fs:(%%edi)\n\t"
        "movb %c[s7](%%esi), %%ah\n\t"
        "movb %c[s6](%%esi), %%al\n\t"
        "shll $16, %%eax\n\t"
        "movb %c[s5](%%esi), %%ah\n\t"
        "movb %c[s4](%%esi), %%al\n\t"
        "movl %%eax, %%fs:4(%%edi)\n\t"
        "addl %[pair], %%esi\n\t"
        "addl $8, %%edi\n\t"
        "decl %%ecx\n\t"
        "jnz 1b"
        : "+S"(source), "+D"(address), "+c"(pairs)
        : [s1] "i"(Stride), [s2] "i"(2 * Stride), [s3] "i"(3 * Stride),
          [s4] "i"(4 * Stride), [s5] "i"(5 * Stride), [s6] "i"(6 * Stride),
          [s7] "i"(7 * Stride), [pair] "i"(8 * Stride)
        : "eax", "cc", "memory");
  }
  if (words % 2 != 0) {
    bmp_write32(address, gatherWord(source, Stride));
    address += PLANES;
    source += PLANES * Stride;
  }
}

void writePlaneRow(uintptr_t address, const uint8_t *source, int count,
                   int step) {
  const int stride = PLANES * step;
  while (count > 0 && address % sizeof(uint32_t) != 0) {
    bmp_write8(address, *source);
    ++address;
    source += stride;
    --count;
  }
  const int words = count / PLANES;
  count -= words * PLANES;
  if (step == 1) {
    writePlaneWords<PLANES>(address, source, words);
  } else if (step == 2) {
    writePlaneWords<2 * PLANES>(address, source, words);
  } else {
    for (int word = 0; word < words; ++word) {
      bmp_write32(address, gatherWord(source, stride));
      address += PLANES;
      source += PLANES * stride;
    }
  }
  for (; count > 0; --count) {
    bmp_write8(address, *source);
    ++address;
    source += stride;
  }
}

int startClock() {
  const int speaker = inportb(SPEAKER_PORT);
  outportb(SPEAKER_PORT, (speaker & ~SPEAKER_DATA) | CLOCK_GATE);
  outportb(TIMER_COMMAND_PORT, CLOCK_MODE);
  outportb(CLOCK_PORT, 0);
  outportb(CLOCK_PORT, 0);
  return speaker;
}

int clockCount() {
  outportb(TIMER_COMMAND_PORT, CLOCK_LATCH);
  const int low = inportb(CLOCK_PORT);
  return low | inportb(CLOCK_PORT) << BYTE_BITS;
}

int clockTicks(int from, int to) { return (from - to) & (CLOCK_WRAP - 1); }

int clockTicks(int from, int to, int vblTicks) {
  const int counted = clockTicks(from, to);
  const int estimate = vblTicks * BPS_TO_TIMER(SCREEN_HERTZ);
  return counted +
         (estimate - counted + CLOCK_WRAP / 2) / CLOCK_WRAP * CLOCK_WRAP;
}

bool isClockRunning() {
  const int start = clockCount();
  for (int read = 0; read < CLOCK_CHECK_READS; ++read) {
    inportb(STATUS_PORT);
  }
  return clockCount() != start;
}

bool isVerticalBlank() {
  int status = inportb(STATUS_PORT);
  if ((status & DISPLAY_OFF) == 0) {
    return false;
  }
  disable();
  const int start = clockCount();
  bool vertical = false;
  while (!vertical && (status & DISPLAY_OFF) != 0) {
    vertical = (status & VERTICAL_RETRACE) != 0 ||
               clockTicks(start, clockCount()) >= VERTICAL_BLANK_TICKS;
    status = inportb(STATUS_PORT);
  }
  enable();
  return vertical;
}

bool waitVerticalBlank(int timeout) {
  while (!isVerticalBlank()) {
    if (vbls >= timeout) {
      return false;
    }
  }
  return true;
}

bool waitDisplay(int timeout) {
  while ((inportb(STATUS_PORT) & DISPLAY_OFF) != 0) {
    if (vbls >= timeout) {
      return false;
    }
  }
  return true;
}

} // namespace

struct VideoSystem::Window {
  void draw();
  void followRetrace();
  void measurePeriod(int found, int vblTicks);
  void scrollTo(int next, int pan);
  void prepareScroll();
  void latch(bool on);
  void writeStrips();

  IndexedRasterizer rasterizer{leadingWords, trailingWords};
  IndexedFrame frame;
  Placement placement;
  bool fresh = true;
  int letterbox = -1;
  int darkest = 0;
  std::array<uint16_t, FRAME_COLORS> palette{};
  PALETTE dac{};
  bool recolored = false;
  bool cleared = false;
  std::vector<RowWork> rows;
  int hertz = 0;
  int nextVbl = 0;
  int speaker = 0;
  bool retrace = false;
  bool timed = false;
  bool polled = false;
  int period = BPS_TO_TIMER(SCREEN_HERTZ);
  int periodSamples = 0;
  int refClock = 0;
  int refVbl = 0;
  int phase = 0;
  int limit = 0;
  int origin = 0;
  int target = 0;
  bool scrolling = false;
  int rebaseFrom = NO_REBASE;
  int rebaseTo = 0;
  int mode = 0;
  bool latched = false;
  bool wide = false;
  Span exposed;
  std::array<std::vector<std::array<int, 2>>, PLANES> strips;
};

VideoSystem::VideoSystem() : m_window(std::make_unique<Window>()) {
  set_color_depth(8);
  if (set_gfx_mode(GFX_MODEX, SCREEN_WIDTH, SCREEN_HEIGHT, VIRTUAL_WIDTH,
                   SCREEN_HEIGHT) != 0 &&
      set_gfx_mode(GFX_MODEX, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0) != 0) {
    throwError("Failed to open the 376x282 VGA screen");
  }
  m_window->limit = std::max(VIRTUAL_W - SCREEN_WIDTH, 0);
  LOCK_VARIABLE(vbls);
  LOCK_FUNCTION(countVbl);
  set_palette(black_palette);
  m_window->speaker = startClock();
  m_window->retrace = isClockRunning();
}

VideoSystem::~VideoSystem() {
  remove_int(countVbl);
  outportb(SPEAKER_PORT, m_window->speaker);
  set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
}

bool VideoSystem::showsSprites() const { return false; }

void VideoSystem::present() {
  if (!m_frameChanged) {
    return;
  }
  m_frameChanged = false;
  Window &window = *m_window;
  IndexedFrame &frame = window.frame;
  window.rasterizer.rasterize(m_shown, frame);
  const Placement placement = place(m_shown);
  const bool fresh = window.fresh || placement != window.placement;
  window.placement = placement;
  window.fresh = false;

  const std::size_t paletteBytes = sizeof(frame.palette);
  if (leadingWords(reinterpret_cast<const uint8_t *>(frame.palette.data()),
                   reinterpret_cast<const uint8_t *>(window.palette.data()),
                   paletteBytes) != paletteBytes) {
    window.palette = frame.palette;
    window.darkest = darkestSlot(frame.palette);
    for (std::size_t slot = 0; slot < FRAME_COLORS; ++slot) {
      window.dac[slot].r =
          static_cast<unsigned char>(dacLevel(frame.palette[slot], 8));
      window.dac[slot].g =
          static_cast<unsigned char>(dacLevel(frame.palette[slot], 4));
      window.dac[slot].b =
          static_cast<unsigned char>(dacLevel(frame.palette[slot], 0));
    }
    window.recolored = true;
  }
  const int letterbox = window.darkest;
  const int step = placement.step;
  const bool movable = frame.width % (PLANES * step) == 0;
  const bool copyable = placement.height == frame.height;
  bool cleared = fresh || letterbox != window.letterbox;
  const int pan =
      !cleared && step == 1 && copyable ? panOf(frame, placement.height) : 0;
  const bool scrolls = pan != 0 && window.limit >= SCREEN_WIDTH;
  cleared = cleared || (!scrolls && window.origin != 0 && movesRows(frame));
  window.exposed = {};
  if (cleared) {
    window.cleared = true;
    window.letterbox = letterbox;
    window.scrollTo(0, 0);
  } else if (scrolls) {
    window.scrollTo(window.origin - pan, pan);
    window.exposed =
        pan < 0 ? Span{frame.width + pan, frame.width} : Span{0, pan};
  }
  const auto screenSpan = [&](const Span &span) {
    return Span{(span.first + step - 1) / step,
                std::min(placement.width, (span.last + step - 1) / step)};
  };
  window.rows.clear();
  for (int row = 0; scrolls && row < placement.height; ++row) {
    RowWork work;
    work.row = row;
    work.spans = frame.changes[static_cast<std::size_t>(row)].spans;
    for (Span &span : work.spans) {
      if (pan < 0) {
        span.last = std::min(span.last, window.exposed.first);
      } else {
        span.first = std::max(span.first, window.exposed.last);
      }
    }
    if (std::any_of(work.spans.begin(), work.spans.end(),
                    [](const Span &span) { return span.first < span.last; })) {
      window.rows.push_back(work);
    }
  }
  for (int row = 0; !scrolls && row < placement.height; ++row) {
    const RowChange &change =
        frame.changes[static_cast<std::size_t>(sourceRow(placement, row))];
    RowWork work;
    work.row = row;
    const bool whole = cleared || (change.from != NO_ROW && !copyable) ||
                       (change.shift != 0 &&
                        (!movable || change.shift % (PLANES * step) != 0));
    if (whole) {
      work.spans[0] = {0, placement.width};
    } else if (isChanged(change)) {
      work.from = change.from;
      work.shift = change.shift / step;
      work.spans = {screenSpan(change.spans[0]), screenSpan(change.spans[1])};
    } else {
      continue;
    }
    window.rows.push_back(work);
  }
  if (std::any_of(window.rows.begin(), window.rows.end(),
                  [](const RowWork &work) {
                    return work.from != NO_ROW && work.from < work.row;
                  })) {
    std::reverse(window.rows.begin(), window.rows.end());
  }
}

void VideoSystem::Window::scrollTo(int next, int pan) {
  if (next < 0 || next > limit) {
    rebaseFrom = origin;
    rebaseTo =
        next > limit ? origin % PLANES : limit - (limit - origin) % PLANES;
    next = rebaseTo - pan;
  }
  target = next;
  scrolling = target != origin || rebaseFrom != NO_REBASE;
}

void VideoSystem::Window::latch(bool on) {
  if (on == latched) {
    return;
  }
  latched = on;
  outportw(GRAPHICS_PORT, ((mode & ~WRITE_MODES) | (on ? LATCH_WRITE : 0))
                                  << PLANE_SHIFT |
                              MODE_REGISTER);
  if (on) {
    outportw(SEQUENCER_PORT, ALL_PLANES << PLANE_SHIFT | MAP_MASK);
  }
}

void VideoSystem::Window::prepareScroll() {
  if (!scrolling) {
    return;
  }
  if (!wide && target != 0) {
    rectfill(screen, SCREEN_WIDTH, 0, VIRTUAL_W - 1, SCREEN_HEIGHT - 1,
             letterbox);
    wide = true;
  }
  if (rebaseFrom != NO_REBASE) {
    bmp_select(screen);
    outportb(GRAPHICS_PORT, MODE_REGISTER);
    mode = inportb(GRAPHICS_PORT + 1);
    latched = false;
    latch(true);
    const int count = SCREEN_WIDTH / PLANES + 1;
    for (int line = 0; line < SCREEN_HEIGHT; ++line) {
      const uintptr_t start = reinterpret_cast<uintptr_t>(screen->line[line]);
      copyLatches(start + static_cast<uintptr_t>(rebaseFrom / PLANES),
                  start + static_cast<uintptr_t>(rebaseTo / PLANES), count,
                  rebaseTo > rebaseFrom);
    }
    latch(false);
    rebaseFrom = NO_REBASE;
  }
  const uintptr_t start = reinterpret_cast<uintptr_t>(screen->line[0]) +
                          static_cast<uintptr_t>(target / PLANES);
  outportb(CRTC_PORT, START_HIGH);
  outportb(CRTC_PORT + 1, static_cast<int>(start >> BYTE_BITS) & 0xFF);
  outportb(CRTC_PORT, START_LOW);
  outportb(CRTC_PORT + 1, static_cast<int>(start) & 0xFF);
}

void VideoSystem::Window::writeStrips() {
  for (std::vector<std::array<int, 2>> &columns : strips) {
    columns.clear();
  }
  const auto add = [&](int column, int source) {
    strips[static_cast<std::size_t>(column & (PLANES - 1))].push_back(
        {column / PLANES, source});
  };
  for (int column = 0; column < placement.x; ++column) {
    add(origin + column, NO_ROW);
  }
  for (int column = exposed.first; column < exposed.last; ++column) {
    add(origin + placement.x + column, column);
  }
  for (int column = placement.x + placement.width; column < SCREEN_WIDTH;
       ++column) {
    add(origin + column, NO_ROW);
  }
  bmp_select(screen);
  const auto width = static_cast<std::size_t>(frame.width);
  for (int plane = 0; plane < PLANES; ++plane) {
    const std::vector<std::array<int, 2>> &columns =
        strips[static_cast<std::size_t>(plane)];
    if (columns.empty()) {
      continue;
    }
    outportw(SEQUENCER_PORT, (1 << (PLANE_SHIFT + plane)) | MAP_MASK);
    for (int row = 0; row < placement.height; ++row) {
      const uintptr_t line =
          reinterpret_cast<uintptr_t>(screen->line[placement.y + row]);
      const uint8_t *source =
          frame.pixels.data() + static_cast<std::size_t>(row) * width;
      for (const std::array<int, 2> &column : columns) {
        bmp_write8(line + static_cast<uintptr_t>(column[0]),
                   column[1] == NO_ROW ? letterbox : source[column[1]]);
      }
    }
  }
}

void VideoSystem::Window::draw() {
  if (scrolling) {
    inportb(STATUS_PORT);
    outportb(ATTRIBUTE_PORT, PEL_PANNING | PALETTE_ADDRESS_SOURCE);
    outportb(ATTRIBUTE_PORT, target % PLANES * PEL_STEP);
    origin = target;
    scrolling = false;
    if (!cleared) {
      writeStrips();
    }
  }
  if (recolored) {
    set_palette_range(dac, 0, static_cast<int>(FRAME_COLORS) - 1, FALSE);
    recolored = false;
  }
  if (cleared) {
    rectfill(screen, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, letterbox);
    wide = false;
    cleared = false;
  }
  if (rows.empty()) {
    return;
  }
  bmp_select(screen);
  outportb(GRAPHICS_PORT, MODE_REGISTER);
  mode = inportb(GRAPHICS_PORT + 1);
  latched = false;
  const int left = origin + placement.x;
  const auto lineStart = [&](int row) {
    return reinterpret_cast<uintptr_t>(screen->line[placement.y + row]);
  };
  const auto rowStart = [&](int row) {
    return lineStart(row) + static_cast<uintptr_t>(left / PLANES);
  };
  const int groups = placement.width / PLANES;
  const std::size_t width = static_cast<std::size_t>(frame.width);
  const int step = placement.step;
  for (std::size_t band = 0; band < rows.size(); band += BAND_ROWS) {
    const std::size_t end = std::min(rows.size(), band + BAND_ROWS);
    for (std::size_t index = band; index < end; ++index) {
      const RowWork &work = rows[index];
      if (work.from != NO_ROW) {
        latch(true);
        copyLatches(rowStart(work.from), rowStart(work.row), groups, false);
      }
      if (work.shift != 0) {
        latch(true);
        const int moved = std::abs(work.shift) / PLANES;
        const uintptr_t start = rowStart(work.row);
        if (work.shift < 0) {
          copyLatches(start + static_cast<uintptr_t>(moved), start,
                      groups - moved, false);
        } else {
          copyLatches(start, start + static_cast<uintptr_t>(moved),
                      groups - moved, true);
        }
      }
    }
    for (int plane = 0; plane < PLANES; ++plane) {
      bool selected = false;
      for (std::size_t index = band; index < end; ++index) {
        const RowWork &work = rows[index];
        const uint8_t *source =
            frame.pixels.data() +
            static_cast<std::size_t>(sourceRow(placement, work.row)) * width;
        for (const Span &span : work.spans) {
          const unsigned first = static_cast<unsigned>(
              left + span.first + ((plane - left - span.first) & (PLANES - 1)));
          const unsigned last = static_cast<unsigned>(left + span.last);
          if (first >= last) {
            continue;
          }
          if (!selected) {
            latch(false);
            outportw(SEQUENCER_PORT, (1 << (PLANE_SHIFT + plane)) | MAP_MASK);
            selected = true;
          }
          writePlaneRow(lineStart(work.row) + first / PLANES,
                        source + (first - static_cast<unsigned>(left)) * step,
                        static_cast<int>((last - first + PLANES - 1) / PLANES),
                        step);
        }
      }
    }
  }
  latch(false);
  rows.clear();
}

void VideoSystem::Window::followRetrace() {
  const int clock = clockCount();
  const int ticks = vbls;
  const bool known = timed && ticks - refVbl <= PHASE_TIMEOUT;
  const int since =
      known ? phase + clockTicks(refClock, clock, ticks - refVbl) : 0;
  if (known && since >= period) {
    phase = since % period;
    refClock = clock;
    refVbl = ticks;
    polled = false;
    return;
  }
  const bool leave = !known || since < period / QUICK_PART;
  if ((leave && !waitDisplay(ticks + RETRACE_TIMEOUT)) ||
      !waitVerticalBlank(ticks + RETRACE_TIMEOUT)) {
    retrace = false;
    return;
  }
  const int found = clockCount();
  if (known && polled) {
    measurePeriod(found, vbls - refVbl);
  }
  timed = true;
  polled = true;
  phase = 0;
  refClock = found;
  refVbl = vbls;
}

void VideoSystem::Window::measurePeriod(int found, int vblTicks) {
  const int elapsed = clockTicks(refClock, found, vblTicks);
  const int refreshes = (elapsed + period / 2) / period;
  if (refreshes == 0) {
    return;
  }
  period += (elapsed / refreshes - period) / PERIOD_SMOOTHING;
  if (++periodSamples == PERIOD_SAMPLES) {
    retrace = period >= BPS_TO_TIMER(FASTEST_HERTZ) &&
              period <= BPS_TO_TIMER(SLOWEST_HERTZ);
  }
}

void VideoSystem::waitVbl() {
  Window &window = *m_window;
  const int hertz = refreshRate();
  if (hertz != window.hertz) {
    install_int_ex(countVbl, BPS_TO_TIMER(hertz));
    window.hertz = hertz;
    window.nextVbl = vbls;
    window.timed = false;
  }
  window.prepareScroll();
  if (window.retrace && hertz == SCREEN_HERTZ) {
    window.followRetrace();
    window.nextVbl = vbls;
  } else {
    ++window.nextVbl;
    while (vbls < window.nextVbl) {
    }
    if (vbls - window.nextVbl > 1) {
      window.nextVbl = vbls;
    }
  }
  window.draw();
}

} // namespace openfranko::src::systems::graphics
