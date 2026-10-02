#include "Blitter.h"

#include "Hardware.h"

#include <algorithm>

namespace openfranko::src::systems::jaguar::blitter {
namespace {

constexpr int PHRASE = 8;
constexpr int LARGEST_X = 16000;
constexpr int MAX_ROWS = 4000;
constexpr int MAX_EXPONENT = 11;
constexpr int CLASS_MIN_ROWS = 16;
constexpr uint32_t WINDOW_WIDTH = (3u << 2) << BLIT_WIDTH_SHIFT;
constexpr uint32_t COPY =
    BLIT_SRCEN | BLIT_UPDA1 | BLIT_UPDA2 | BLIT_LFU_SOURCE;
constexpr uint32_t FILL = BLIT_PATDSEL | BLIT_UPDA1;
constexpr uint32_t MASKED_PHRASES = COPY | BLIT_DSTEN | BLIT_DCOMPEN;
constexpr uint32_t MASKED_PIXELS = COPY | BLIT_DCOMPEN;
constexpr uint32_t QUEUE_ENTRIES = 64;
constexpr uint32_t ENTRY_LONGS = 16;
constexpr uint32_t INDEX_MASK = 0xFFFF;
constexpr uint32_t QUEUE_WRITE_OFFSET = 4;
constexpr uint32_t QUEUE_READ_OFFSET = 8;
constexpr uint32_t KIND_PHRASES = 1;
constexpr uint32_t KIND_PIXELS = 2;
constexpr uint32_t KIND_FILL = 3;
constexpr uint32_t KIND_LZ4 = 4;
constexpr uint32_t KIND_OUTLINE = 5;
constexpr uint32_t KIND_FLIP = 6;
constexpr std::size_t OUTLINE_BOX = 6;
constexpr std::size_t BOX_VALUES = 4;
constexpr uint32_t SIGN_BITS = 0x80808080u;
constexpr std::size_t LONG_BYTES = 4;
constexpr std::size_t UNPACKED_END = 5;

alignas(64) volatile uint32_t ring[QUEUE_ENTRIES * ENTRY_LONGS];
volatile uint32_t *queueWrite = nullptr;
volatile uint32_t *queueRead = nullptr;
uint32_t written = 0;
uint32_t pattern = 0;

struct Channel {
  uint32_t base = 0;
  uint32_t flags = 0;
  uint32_t pixel = 0;
  uint32_t step = 0;
};

int product(int16_t left, int16_t right) {
  int32_t result = left;
  asm("muls.w %1,%0" : "+d"(result) : "d"(right) : "cc");
  return result;
}

int magnitude(int value) { return value < 0 ? -value : value; }

uint32_t phraseBase(const uint8_t *pixels) {
  return reinterpret_cast<uint32_t>(pixels) & ~uint32_t(PHRASE - 1);
}

int phraseOffset(const uint8_t *pixels) {
  return static_cast<int>(reinterpret_cast<uint32_t>(pixels) & (PHRASE - 1));
}

uint32_t point(int x, int y) {
  return static_cast<uint32_t>(y) << 16 | (static_cast<uint32_t>(x) & 0xFFFF);
}

int phraseEnd(int start, int width) {
  return (start + width + PHRASE - 1) & ~(PHRASE - 1);
}

int widthCode(int pitch) {
  if (pitch < PHRASE || pitch % PHRASE != 0) {
    return -1;
  }
  int exponent = 0;
  while ((pitch >> exponent) > 7) {
    ++exponent;
  }
  if ((pitch & ((1 << exponent) - 1)) != 0 || exponent + 2 > MAX_EXPONENT) {
    return -1;
  }
  return (exponent + 2) << 2 | ((pitch >> exponent) & 3);
}

int linearRows(int width, int span, int remaining) {
  const int room = LARGEST_X - 2 * PHRASE - width;
  if (span == 0 || room < 0) {
    return std::clamp(span == 0 ? remaining : 1, 1, MAX_ROWS);
  }
  const int rows =
      static_cast<uint16_t>(room) / static_cast<uint16_t>(span) + 1;
  return std::clamp(rows, 1, std::min(remaining, MAX_ROWS));
}

uint32_t flagsFor(int code, uint32_t addressing) {
  return BLIT_PITCH1 | BLIT_PIXEL8 | addressing |
         (code >= 0 ? static_cast<uint32_t>(code) << BLIT_WIDTH_SHIFT
                    : WINDOW_WIDTH);
}

void setPattern(uint8_t value) { pattern = value * 0x01010101u; }

uint32_t pending() { return (written - *queueRead) & INDEX_MASK; }

bool aligned(int pitch) { return (pitch & (PHRASE - 1)) == 0; }

int periodShift(int pitch) {
  int remainder = magnitude(pitch) & (PHRASE - 1);
  int shift = 3;
  while (remainder != 0 && (remainder & 1) == 0) {
    remainder >>= 1;
    --shift;
  }
  return remainder == 0 ? 0 : shift;
}

void enqueue(uint32_t kind, const uint8_t *source, int sourcePitch,
             uint8_t *target, int targetPitch, int width, int height,
             uint32_t command, bool mirrored) {
  if (width <= 0 || height <= 0) {
    return;
  }
  while (pending() >= QUEUE_ENTRIES) {
  }
  volatile uint32_t *entry =
      ring + (written & (QUEUE_ENTRIES - 1)) * ENTRY_LONGS;
  entry[0] = kind;
  entry[1] = reinterpret_cast<uint32_t>(source);
  entry[2] = static_cast<uint32_t>(sourcePitch);
  entry[3] = reinterpret_cast<uint32_t>(target);
  entry[4] = static_cast<uint32_t>(targetPitch);
  entry[5] = static_cast<uint32_t>(width);
  entry[6] = static_cast<uint32_t>(height);
  entry[7] = command;
  entry[8] = mirrored ? 1u : 0u;
  entry[9] = pattern;
  written = (written + 1) & INDEX_MASK;
  *queueWrite = written;
}

void start(const Channel &target, const Channel *source, int rows, int width,
           uint32_t command) {
  const uint32_t count =
      static_cast<uint32_t>(rows) << 16 | static_cast<uint32_t>(width);
  wait();
  longWord(A1_BASE) = target.base;
  longWord(A1_FLAGS) = target.flags;
  longWord(A1_CLIP) = 0;
  longWord(A1_PIXEL) = target.pixel;
  longWord(A1_STEP) = target.step;
  if (source) {
    longWord(A2_BASE) = source->base;
    longWord(A2_FLAGS) = source->flags;
    longWord(A2_PIXEL) = source->pixel;
    longWord(A2_STEP) = source->step;
  }
  longWord(B_PATD) = pattern;
  longWord(B_PATD + 4) = pattern;
  longWord(B_COUNT) = count;
  longWord(B_CMD) = command;
}

void settle() {
  if (!queueWrite) {
    wait();
  }
}

void phrases(Source source, Area target, int width, int height,
             uint32_t command) {
  if (queueWrite) {
    enqueue(KIND_PHRASES, source.pixels, source.pitch, target.pixels,
            target.pitch, width, height, command, false);
    return;
  }
  const int sourceCode = widthCode(source.pitch);
  const int targetCode = widthCode(target.pitch);
  const bool stepped = sourceCode >= 0 && targetCode >= 0;
  const int span = std::max(magnitude(source.pitch), target.pitch);
  const int sourceX = phraseOffset(source.pixels);
  const int targetX = phraseOffset(target.pixels);
  const bool extraRead = sourceX > targetX;
  const int targetEnd = phraseEnd(targetX, width);
  const int sourceEnd = targetEnd + (extraRead ? PHRASE : 0);
  Channel to;
  Channel from;
  to.flags = flagsFor(stepped ? targetCode : -1, BLIT_XADDPHR);
  from.flags = flagsFor(stepped ? sourceCode : -1, BLIT_XADDPHR);
  to.pixel = point(targetX, 0);
  if (stepped) {
    from.pixel = point(sourceX, 0);
    to.step = point(targetX - targetEnd, 1);
    from.step = point(sourceX - sourceEnd, 1);
  } else {
    to.step = point(targetX + target.pitch - targetEnd, 0);
    from.step = point(sourceX + source.pitch - sourceEnd, 0);
  }
  if (extraRead) {
    command |= BLIT_SRCENX;
  }
  const uint8_t *fromRow = source.pixels;
  uint8_t *toRow = target.pixels;
  int row = 0;
  while (row < height) {
    const int rows = stepped ? std::min(height - row, MAX_ROWS)
                             : linearRows(width, span, height - row);
    const int sourceSpan = product(static_cast<int16_t>(rows - 1),
                                   static_cast<int16_t>(source.pitch));
    const uint8_t *lowest = source.pitch < 0 ? fromRow + sourceSpan : fromRow;
    to.base = phraseBase(toRow);
    from.base = phraseBase(lowest);
    if (!stepped) {
      from.pixel = point(static_cast<int>(fromRow - lowest) + sourceX, 0);
    }
    start(to, &from, rows, width, command);
    fromRow += sourceSpan + source.pitch;
    toRow +=
        product(static_cast<int16_t>(rows), static_cast<int16_t>(target.pitch));
    row += rows;
  }
}

void pixels(Source source, Area target, int width, int height, uint32_t command,
            bool mirrored) {
  if (queueWrite) {
    enqueue(KIND_PIXELS, source.pixels, source.pitch, target.pixels,
            target.pitch, width, height, command, mirrored);
    return;
  }
  const int span = std::max(magnitude(source.pitch), target.pitch);
  Channel to;
  Channel from;
  to.flags = flagsFor(-1, BLIT_XADDPIX | (mirrored ? BLIT_XSIGNSUB : 0));
  to.step = point(mirrored ? target.pitch + width : target.pitch - width, 0);
  from.flags = flagsFor(-1, BLIT_XADDPIX);
  from.step = point(source.pitch - width, 0);
  const uint8_t *fromRow = source.pixels;
  uint8_t *toRow = target.pixels;
  int row = 0;
  while (row < height) {
    const int rows = linearRows(width, span, height - row);
    const int sourceSpan = product(static_cast<int16_t>(rows - 1),
                                   static_cast<int16_t>(source.pitch));
    const uint8_t *lowest = source.pitch < 0 ? fromRow + sourceSpan : fromRow;
    to.base = phraseBase(toRow);
    to.pixel = point(phraseOffset(toRow) + (mirrored ? width - 1 : 0), 0);
    from.base = phraseBase(lowest);
    from.pixel =
        point(static_cast<int>(fromRow - lowest) + phraseOffset(lowest), 0);
    start(to, &from, rows, width, command);
    fromRow += sourceSpan + source.pitch;
    toRow +=
        product(static_cast<int16_t>(rows), static_cast<int16_t>(target.pitch));
    row += rows;
  }
}

void transfer(Source source, Area target, int width, int height,
              uint32_t phraseCommand, uint32_t pixelCommand) {
  if (width <= 0 || height <= 0) {
    return;
  }
  if (aligned(source.pitch) && aligned(target.pitch)) {
    phrases(source, target, width, height, phraseCommand);
    return;
  }
  const int shift =
      std::max(periodShift(source.pitch), periodShift(target.pitch));
  const int period = 1 << shift;
  if (height < period * CLASS_MIN_ROWS) {
    pixels(source, target, width, height, pixelCommand, false);
    return;
  }
  const int16_t sourceStride = static_cast<int16_t>(source.pitch << shift);
  const int16_t targetStride = static_cast<int16_t>(target.pitch << shift);
  for (int first = 0; first < period; ++first) {
    phrases(Source{source.pixels + product(static_cast<int16_t>(first),
                                           static_cast<int16_t>(source.pitch)),
                   sourceStride},
            Area{target.pixels + product(static_cast<int16_t>(first),
                                         static_cast<int16_t>(target.pitch)),
                 targetStride},
            width, (height - first + period - 1) >> shift, phraseCommand);
  }
}

void fillPhrases(Area target, int width, int height) {
  if (queueWrite) {
    enqueue(KIND_FILL, nullptr, 0, target.pixels, target.pitch, width, height,
            FILL, false);
    return;
  }
  const int code = widthCode(target.pitch);
  const bool stepped = code >= 0;
  const int targetX = phraseOffset(target.pixels);
  const int targetEnd = phraseEnd(targetX, width);
  Channel to;
  to.flags = flagsFor(code, BLIT_XADDPHR);
  to.pixel = point(targetX, 0);
  to.step = stepped ? point(targetX - targetEnd, 1)
                    : point(targetX + target.pitch - targetEnd, 0);
  uint8_t *toRow = target.pixels;
  int row = 0;
  while (row < height) {
    const int rows = stepped ? std::min(height - row, MAX_ROWS)
                             : linearRows(width, target.pitch, height - row);
    to.base = phraseBase(toRow);
    start(to, nullptr, rows, width, FILL);
    toRow +=
        product(static_cast<int16_t>(rows), static_cast<int16_t>(target.pitch));
    row += rows;
  }
}

} // namespace

void wait() {
  if (queueWrite) {
    while (pending() != 0) {
    }
  }
  while ((longWord(B_CMD) & BLIT_IDLE) == 0) {
  }
  asm volatile("" ::: "memory");
}

void useQueue(uint32_t control) {
  wait();
  written = 0;
  longWord(control) = reinterpret_cast<uint32_t>(ring);
  longWord(control + QUEUE_WRITE_OFFSET) = 0;
  longWord(control + QUEUE_READ_OFFSET) = 0;
  queueRead = &longWord(control + QUEUE_READ_OFFSET);
  queueWrite = &longWord(control + QUEUE_WRITE_OFFSET);
}

bool isQueued() { return queueWrite != nullptr; }

bool unpack(const uint8_t *source, std::size_t sourceSize, uint8_t *target,
            std::size_t targetSize) {
  if (!queueWrite) {
    return false;
  }
  while (pending() >= QUEUE_ENTRIES) {
  }
  volatile uint32_t *entry =
      ring + (written & (QUEUE_ENTRIES - 1)) * ENTRY_LONGS;
  const uint32_t end = reinterpret_cast<uint32_t>(target + targetSize);
  entry[0] = KIND_LZ4;
  entry[1] = reinterpret_cast<uint32_t>(source);
  entry[2] = reinterpret_cast<uint32_t>(source + sourceSize);
  entry[3] = reinterpret_cast<uint32_t>(target);
  entry[4] = end;
  entry[UNPACKED_END] = 0;
  written = (written + 1) & INDEX_MASK;
  *queueWrite = written;
  wait();
  return entry[UNPACKED_END] == end;
}

bool outline(const uint8_t *pixels, int width, int height, void *rows,
             void *bands, int32_t *box) {
  if (!queueWrite) {
    return false;
  }
  while (pending() >= QUEUE_ENTRIES) {
  }
  volatile uint32_t *entry =
      ring + (written & (QUEUE_ENTRIES - 1)) * ENTRY_LONGS;
  entry[0] = KIND_OUTLINE;
  entry[1] = reinterpret_cast<uint32_t>(pixels);
  entry[2] = static_cast<uint32_t>(width);
  entry[3] = static_cast<uint32_t>(height);
  entry[4] = reinterpret_cast<uint32_t>(rows);
  entry[5] = reinterpret_cast<uint32_t>(bands);
  written = (written + 1) & INDEX_MASK;
  *queueWrite = written;
  wait();
  for (std::size_t value = 0; value < BOX_VALUES; ++value) {
    box[value] = static_cast<int32_t>(entry[OUTLINE_BOX + value]);
  }
  return true;
}

bool flipSigns(const uint8_t *source, int8_t *target, std::size_t count) {
  const uint32_t misaligned = (reinterpret_cast<uint32_t>(source) |
                               reinterpret_cast<uint32_t>(target)) &
                              (LONG_BYTES - 1);
  if (!queueWrite || misaligned != 0) {
    return false;
  }
  const std::size_t longs = count / LONG_BYTES;
  while (pending() >= QUEUE_ENTRIES) {
  }
  volatile uint32_t *entry =
      ring + (written & (QUEUE_ENTRIES - 1)) * ENTRY_LONGS;
  entry[0] = KIND_FLIP;
  entry[1] = reinterpret_cast<uint32_t>(source);
  entry[2] = reinterpret_cast<uint32_t>(target);
  entry[3] = static_cast<uint32_t>(longs);
  entry[4] = SIGN_BITS;
  written = (written + 1) & INDEX_MASK;
  *queueWrite = written;
  for (std::size_t at = longs * LONG_BYTES; at < count; ++at) {
    target[at] = static_cast<int8_t>(source[at] ^ 0x80u);
  }
  wait();
  return true;
}

void stopQueue() {
  wait();
  queueWrite = nullptr;
  queueRead = nullptr;
}

void copy(Source source, Area target, int width, int height) {
  transfer(source, target, width, height, COPY, COPY);
  settle();
}

void copyMasked(Source source, Area target, int width, int height) {
  setPattern(0);
  transfer(source, target, width, height, MASKED_PHRASES, MASKED_PIXELS);
  settle();
}

void copyMirrored(Source source, Area target, int width, int height,
                  bool masked) {
  if (width <= 0 || height <= 0) {
    return;
  }
  setPattern(0);
  pixels(source, target, width, height, masked ? MASKED_PIXELS : COPY, true);
  settle();
}

void fill(Area target, int width, int height, uint8_t value) {
  if (width <= 0 || height <= 0) {
    return;
  }
  setPattern(value);
  const int shift = periodShift(target.pitch);
  const int period = 1 << shift;
  if (period == 1) {
    fillPhrases(target, width, height);
  } else {
    const int16_t stride = static_cast<int16_t>(target.pitch << shift);
    for (int first = 0; first < period && first < height; ++first) {
      fillPhrases(
          Area{target.pixels + product(static_cast<int16_t>(first),
                                       static_cast<int16_t>(target.pitch)),
               stride},
          width, (height - first + period - 1) >> shift);
    }
  }
  settle();
}

} // namespace openfranko::src::systems::jaguar::blitter
