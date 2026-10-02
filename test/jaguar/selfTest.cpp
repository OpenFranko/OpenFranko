#include "../../lib/converter/packedArchive/lz4Compressor.h"
#include "../../src/systems/jaguar/Blitter.h"
#include "../../src/systems/jaguar/Console.h"
#include "../../src/systems/jaguar/Eeprom.h"
#include "../../src/systems/jaguar/Hardware.h"
#include "../../src/systems/jaguar/Profiler.h"
#include "../../src/systems/jaguar/RiscProgram.h"
#include "../../src/systems/jaguar/Runtime.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace openfranko::src::systems::jaguar;

namespace {

constexpr uint8_t UNTOUCHED = 0xEE;
constexpr uint32_t EEPROM_VBLS = 600;

enum class Kind { Copy, Masked, Mirrored, MirroredMasked };

struct Case {
  const char *name;
  Kind kind;
  int sourceX;
  int targetX;
  int width;
  int height;
  int sourcePitch;
  int targetPitch;
  bool timed;
};

uint8_t sample(int x, int y) {
  const int value = (x * 7 + y * 13 + 1) & 0xFF;
  return static_cast<uint8_t>(((x + 3 * y) & 7) == 0 ? 0 : value);
}

int failures = 0;
int passes = 0;
bool queueCalls = false;

void report(const std::string &line) { console::print(line); }

bool same(const Case &test, const std::vector<uint8_t> &target,
          const std::vector<uint8_t> &expected) {
  for (std::size_t i = 0; i < target.size(); ++i) {
    if (target[i] != expected[i]) {
      char line[96];
      std::snprintf(line, sizeof(line), "%s: FAIL at %d,%d got %02X want %02X",
                    test.name, static_cast<int>(i % test.targetPitch),
                    static_cast<int>(i / test.targetPitch), target[i],
                    expected[i]);
      report(line);
      ++failures;
      return false;
    }
  }
  ++passes;
  return true;
}

void runCopy(const Case &test) {
  const int rows = test.height + 1;
  const int span = std::abs(test.sourcePitch);
  std::vector<uint8_t> source(static_cast<std::size_t>(span * rows + 16));
  std::vector<uint8_t> target(static_cast<std::size_t>(test.targetPitch * rows),
                              UNTOUCHED);
  std::size_t at = 0;
  for (int y = 0; at < source.size(); ++y) {
    for (int x = 0; x < span && at < source.size(); ++x) {
      source[at++] = sample(x, y);
    }
  }
  const uint8_t *first = source.data() + test.sourceX +
                         (test.sourcePitch < 0 ? (test.height - 1) * span : 0);
  const bool masked =
      test.kind == Kind::Masked || test.kind == Kind::MirroredMasked;
  const bool mirrored =
      test.kind == Kind::Mirrored || test.kind == Kind::MirroredMasked;
  std::vector<uint8_t> expected = target;
  for (int y = 0; y < test.height; ++y) {
    for (int x = 0; x < test.width; ++x) {
      const uint8_t value =
          first[y * test.sourcePitch + (mirrored ? test.width - 1 - x : x)];
      if (!masked || value != 0) {
        expected[static_cast<std::size_t>(y * test.targetPitch + test.targetX +
                                          x)] = value;
      }
    }
  }
  const blitter::Source from{first, test.sourcePitch};
  const blitter::Area to{target.data() + test.targetX, test.targetPitch};
  const uint16_t start = profiler::now();
  if (queueCalls) {
    const blitter::Mode modes[] = {blitter::Mode::Copy, blitter::Mode::Masked,
                                   blitter::Mode::Mirrored,
                                   blitter::Mode::MirroredMasked};
    if (!blitter::queue(from, to, test.width, test.height,
                        modes[static_cast<int>(test.kind)])) {
      report(std::string(test.name) + ": not queued");
      ++failures;
      return;
    }
  } else {
    switch (test.kind) {
    case Kind::Copy:
      blitter::copy(from, to, test.width, test.height);
      break;
    case Kind::Masked:
      blitter::copyMasked(from, to, test.width, test.height);
      break;
    case Kind::Mirrored:
    case Kind::MirroredMasked:
      blitter::copyMirrored(from, to, test.width, test.height, masked);
      break;
    }
  }
  blitter::wait();
  const uint16_t ticks = profiler::since(start);
  if (same(test, target, expected) && test.timed) {
    char line[96];
    std::snprintf(line, sizeof(line), "%s: OK %u0 us", test.name,
                  static_cast<unsigned>(ticks));
    report(line);
  }
}

void runFill(const char *name, int targetX, int width, int height, int pitch) {
  const Case test{name,   Kind::Copy, 0,     targetX, width,
                  height, pitch,      pitch, false};
  std::vector<uint8_t> target(static_cast<std::size_t>(pitch * (height + 1)),
                              UNTOUCHED);
  std::vector<uint8_t> expected = target;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      expected[static_cast<std::size_t>(y * pitch + targetX + x)] = 0x5A;
    }
  }
  blitter::fill({target.data() + targetX, pitch}, width, height, 0x5A);
  blitter::wait();
  same(test, target, expected);
}

constexpr std::size_t MEMORY_BYTES = 6144;

void fillPattern(volatile uint8_t *buffer, std::size_t size, uint8_t seed) {
  uint8_t value = seed;
  for (std::size_t at = 0; at < size; ++at) {
    buffer[at] = value;
    value = static_cast<uint8_t>(value * 5 + 1);
  }
}

bool sameBytes(const volatile uint8_t *left, const volatile uint8_t *right,
               std::size_t size) {
  for (std::size_t at = 0; at < size; ++at) {
    if (left[at] != right[at]) {
      return false;
    }
  }
  return true;
}

void testMemory() {
  static uint8_t buffer[MEMORY_BYTES];
  static uint8_t expected[MEMORY_BYTES];
  const std::size_t sizes[] = {0,  1,  2,  3,  7,  8,   15,   16,   47,
                               48, 49, 63, 64, 65, 100, 1024, 2049, 4500};
  int failed = 0;
  for (const std::size_t size : sizes) {
    for (int sourceOffset = 0; sourceOffset < 4; ++sourceOffset) {
      for (int targetOffset = 0; targetOffset < 4; ++targetOffset) {
        for (int mode = 0; mode < 4; ++mode) {
          volatile uint8_t *vb = buffer;
          volatile uint8_t *ve = expected;
          std::size_t source = 8 + static_cast<std::size_t>(sourceOffset);
          std::size_t target = 8 + static_cast<std::size_t>(targetOffset);
          if (mode == 0) {
            target += size + 16;
          } else if (mode == 1) {
            target += size / 3;
          } else if (mode == 2) {
            source += size / 3;
          }
          if (target + size > MEMORY_BYTES || source + size > MEMORY_BYTES) {
            continue;
          }
          const std::size_t window =
              std::min(MEMORY_BYTES, std::max(source, target) + size + 16);
          fillPattern(vb, window, static_cast<uint8_t>(size + mode));
          fillPattern(ve, window, static_cast<uint8_t>(size + mode));
          if (mode == 3) {
            for (std::size_t at = 0; at < size; ++at) {
              ve[target + at] = 0xA5;
            }
            std::memset(buffer + target, 0xA5, size);
          } else {
            if (target > source) {
              for (std::size_t at = size; at > 0; --at) {
                ve[target + at - 1] = ve[source + at - 1];
              }
            } else {
              for (std::size_t at = 0; at < size; ++at) {
                ve[target + at] = ve[source + at];
              }
            }
            if (mode == 0) {
              std::memcpy(buffer + target, buffer + source, size);
            } else {
              std::memmove(buffer + target, buffer + source, size);
            }
          }
          if (!sameBytes(vb, ve, window)) {
            if (failed < 4) {
              char line[96];
              std::snprintf(line, sizeof(line),
                            "memory mode %d size %u src+%d dst+%d: FAIL", mode,
                            static_cast<unsigned>(size), sourceOffset,
                            targetOffset);
              report(line);
            }
            ++failed;
          }
        }
      }
    }
  }
  for (std::size_t size = 0; size < 40; ++size) {
    for (int offset = 0; offset < 3; ++offset) {
      for (int position = -1; position < static_cast<int>(size); ++position) {
        volatile uint8_t *vb = buffer;
        volatile uint8_t *ve = expected;
        fillPattern(vb, 64, 7);
        fillPattern(ve, 64, 7);
        const std::size_t left = 4 + static_cast<std::size_t>(offset);
        int want = 0;
        if (position >= 0) {
          ve[left + static_cast<std::size_t>(position)] = static_cast<uint8_t>(
              vb[left + static_cast<std::size_t>(position)] ^ 0x81);
          want = vb[left + static_cast<std::size_t>(position)] -
                 ve[left + static_cast<std::size_t>(position)];
        }
        const int got = std::memcmp(buffer + left, expected + left, size);
        if ((got < 0) != (want < 0) || (got > 0) != (want > 0)) {
          if (failed < 4) {
            char line[64];
            std::snprintf(line, sizeof(line),
                          "memcmp size %u off %d pos %d FAIL",
                          static_cast<unsigned>(size), offset, position);
            report(line);
          }
          ++failed;
        }
      }
    }
  }
  if (failed) {
    ++failures;
  } else {
    ++passes;
  }
}

std::vector<uint8_t> lz4Sample(std::size_t size, int kind) {
  std::vector<uint8_t> data(size, 0);
  uint32_t seed = static_cast<uint32_t>(size) * 2654435761u + kind;
  for (std::size_t at = 0; at < size; ++at) {
    seed = seed * 1103515245u + 12345u;
    const uint8_t noise = static_cast<uint8_t>(seed >> 24);
    switch (kind) {
    case 0:
      data[at] = at % 64 < 40 ? static_cast<uint8_t>(at % 7) : noise;
      break;
    case 1:
      data[at] = at % 97 == 0 || at % 48 == 13 ? noise : 0;
      break;
    case 2:
      data[at] = noise;
      break;
    default:
      data[at] = static_cast<uint8_t>(at < size / 2 ? 9 : at % 5);
      break;
    }
  }
  return data;
}

void testLz4() {
  const std::size_t sizes[] = {0, 1, 15, 300, 3552, 9000};
  for (int kind = 0; kind < 4; ++kind) {
    for (const std::size_t size : sizes) {
      const std::vector<uint8_t> data = lz4Sample(size, kind);
      const std::vector<uint8_t> packed =
          openfranko::lib::converter::packedArchive::compressLz4(data.data(),
                                                                 data.size());
      std::vector<uint8_t> unpacked(size + 8, UNTOUCHED);
      const bool done =
          blitter::unpack(packed.data(), packed.size(), unpacked.data(), size);
      bool same = done;
      for (std::size_t at = 0; same && at < size; ++at) {
        same = unpacked[at] == data[at];
      }
      for (std::size_t at = size; same && at < unpacked.size(); ++at) {
        same = unpacked[at] == UNTOUCHED;
      }
      if (!same) {
        char line[64];
        std::snprintf(line, sizeof(line), "lz4 kind %d size %u: FAIL", kind,
                      static_cast<unsigned>(size));
        report(line);
        ++failures;
      } else {
        ++passes;
      }
    }
  }
  for (int kind = 0; kind < 4; ++kind) {
    const std::vector<uint8_t> data = lz4Sample(9000, kind);
    const std::vector<uint8_t> packed =
        openfranko::lib::converter::packedArchive::compressLz4(data.data(),
                                                               data.size());
    for (const std::size_t chunk : {1u, 64u, 1000u, 5000u}) {
      std::vector<uint8_t> unpacked(data.size() + 8, UNTOUCHED);
      const uint8_t *source = packed.data();
      const uint8_t *sourceEnd = packed.data() + packed.size();
      uint8_t *target = unpacked.data();
      const uint8_t *end = unpacked.data() + data.size();
      bool same = true;
      for (int call = 0; same && source < sourceEnd; ++call) {
        const uint8_t *limit = std::min<const uint8_t *>(target + chunk, end);
        same = call < 10000 &&
               blitter::unpackPart(source, sourceEnd, target, end, limit) &&
               (target >= limit || source == sourceEnd);
      }
      same = same && target == end;
      for (std::size_t at = 0; same && at < data.size(); ++at) {
        same = unpacked[at] == data[at];
      }
      for (std::size_t at = data.size(); same && at < unpacked.size(); ++at) {
        same = unpacked[at] == UNTOUCHED;
      }
      if (!same) {
        char line[64];
        std::snprintf(line, sizeof(line), "lz4 part kind %d chunk %u: FAIL",
                      kind, static_cast<unsigned>(chunk));
        report(line);
        ++failures;
      } else {
        ++passes;
      }
    }
  }
  const std::vector<uint8_t> data = lz4Sample(3552, 1);
  std::vector<uint8_t> packed =
      openfranko::lib::converter::packedArchive::compressLz4(data.data(),
                                                             data.size());
  packed.resize(packed.size() / 2);
  std::vector<uint8_t> unpacked(data.size() + 8, UNTOUCHED);
  const bool done = blitter::unpack(packed.data(), packed.size(),
                                    unpacked.data(), data.size());
  bool guarded = !done;
  for (std::size_t at = data.size(); at < unpacked.size(); ++at) {
    guarded = guarded && unpacked[at] == UNTOUCHED;
  }
  report(guarded ? "lz4 truncated: rejected" : "lz4 truncated: FAIL");
  guarded ? ++passes : ++failures;
}

uint32_t packSpan(int first, int last) {
  return static_cast<uint32_t>(first) << 16 |
         (static_cast<uint32_t>(last) & 0xFFFFu);
}

void testOutline() {
  const int widths[] = {0, 1, 2, 3, 7, 16, 33, 48, 79, 320};
  const int heights[] = {0, 1, 3, 5, 74};
  uint32_t seed = 12345;
  for (const int width : widths) {
    for (const int height : heights) {
      std::vector<uint8_t> pixels(static_cast<std::size_t>(width) *
                                      static_cast<std::size_t>(height),
                                  0);
      for (std::size_t at = 0; at < pixels.size(); ++at) {
        seed = seed * 1103515245u + 12345u;
        const int row = width == 0 ? 0 : static_cast<int>(at) / width;
        if (row % 4 != 1 && row != 0 && (seed >> 24) % 5 == 0) {
          pixels[at] = static_cast<uint8_t>(seed >> 16);
        }
      }
      const std::size_t count = static_cast<std::size_t>(height);
      std::vector<uint32_t> rows(count + 1, 0xDEADBEEFu);
      std::vector<uint32_t> bands(count + 1, 0xDEADBEEFu);
      int32_t box[4] = {-9, -9, -9, -9};
      const bool done = blitter::outline(pixels.data(), width, height,
                                         rows.data(), bands.data(), box);
      std::vector<int> firsts(count, 0x7FFF);
      std::vector<int> lasts(count, -0x8000);
      int left = 0x7FFF;
      int right = -0x8000;
      int top = -1;
      int bottom = 0;
      for (int row = 0; row < height; ++row) {
        for (int x = 0; x < width; ++x) {
          if (pixels[static_cast<std::size_t>(row * width + x)] != 0) {
            const std::size_t at = static_cast<std::size_t>(row);
            firsts[at] = std::min(firsts[at], x);
            lasts[at] = std::max(lasts[at], x);
          }
        }
        const std::size_t at = static_cast<std::size_t>(row);
        if (firsts[at] <= lasts[at]) {
          left = std::min(left, firsts[at]);
          right = std::max(right, lasts[at] + 1);
          top = top < 0 ? row : top;
          bottom = row + 1;
        }
      }
      if (top < 0) {
        left = 0;
        right = 0;
        top = 0;
        bottom = 0;
      }
      bool same = done && rows[count] == 0xDEADBEEFu &&
                  bands[count] == 0xDEADBEEFu && box[0] == left &&
                  box[1] == top && box[2] == right && box[3] == bottom;
      for (std::size_t row = 0; same && row < count; ++row) {
        int first = 0x7FFF;
        int last = -0x8000;
        for (std::size_t inside = row; inside < count && inside < row + 4;
             ++inside) {
          first = std::min(first, firsts[inside]);
          last = std::max(last, lasts[inside]);
        }
        same = rows[row] == packSpan(firsts[row], lasts[row]) &&
               bands[row] == packSpan(first, last);
      }
      if (!same) {
        char line[64];
        std::snprintf(line, sizeof(line), "outline %dx%d: FAIL", width, height);
        report(line);
        ++failures;
      } else {
        ++passes;
      }
    }
  }
}

void testFlip() {
  const std::size_t sizes[] = {0, 3, 4, 5, 64, 6453, 27570};
  for (const std::size_t size : sizes) {
    std::vector<uint8_t> source(size + 8);
    for (std::size_t at = 0; at < source.size(); ++at) {
      source[at] = static_cast<uint8_t>(at * 37 + size);
    }
    std::vector<int8_t> target(size + 8, 0x55);
    const bool done = blitter::flipSigns(source.data(), target.data(), size);
    bool same = done;
    for (std::size_t at = 0; same && at < size; ++at) {
      same = target[at] == static_cast<int8_t>(source[at] - 128);
    }
    for (std::size_t at = size; same && at < target.size(); ++at) {
      same = target[at] == 0x55;
    }
    if (!same) {
      char line[64];
      std::snprintf(line, sizeof(line), "flip %u: FAIL",
                    static_cast<unsigned>(size));
      report(line);
      ++failures;
    } else {
      ++passes;
    }
  }
}

void testTranslate() {
  const std::size_t sizes[] = {0, 3, 4, 7, 64, 10243};
  const uint32_t masks[][2] = {{0x0F0F0F0Fu, 0x80808080u},
                               {0x1F1F1F1Fu, 0x40404040u},
                               {0xFFFFFFFFu, 0xC0C0C0C0u}};
  for (const std::size_t size : sizes) {
    for (const auto &mask : masks) {
      std::vector<uint8_t> source(size + 8);
      for (std::size_t at = 0; at < source.size(); ++at) {
        source[at] = static_cast<uint8_t>(at * 53 + size);
      }
      std::vector<uint8_t> target(size + 8, 0x55);
      const bool done = blitter::translate(source.data(), target.data(), size,
                                           mask[0], mask[1]);
      blitter::wait();
      bool same = done;
      for (std::size_t at = 0; same && at < size; ++at) {
        same = target[at] == ((source[at] & static_cast<uint8_t>(mask[0])) ^
                              static_cast<uint8_t>(mask[1]));
      }
      for (std::size_t at = size; same && at < target.size(); ++at) {
        same = target[at] == 0x55;
      }
      if (!same) {
        char line[64];
        std::snprintf(line, sizeof(line), "translate %u %08lx: FAIL",
                      static_cast<unsigned>(size),
                      static_cast<unsigned long>(mask[0]));
        report(line);
        ++failures;
      } else {
        ++passes;
      }
    }
  }
}

void testEeprom() {
  eeprom::Bank before{};
  eeprom::readBank(before);
  eeprom::Bank bank{};
  for (std::size_t index = 0; index < bank.size(); ++index) {
    bank[index] = static_cast<uint16_t>(before[0] + 0x0101 + index * 0x0F1F);
  }
  eeprom::queueBank(bank);
  const uint32_t start = runtime::vblCount();
  while (eeprom::isWriting() && runtime::vblCount() - start < EEPROM_VBLS) {
    const uint32_t vbl = runtime::vblCount();
    while (runtime::vblCount() == vbl) {
    }
    eeprom::poll();
  }
  eeprom::Bank after{};
  const bool valid = eeprom::readBank(after);
  char line[64];
  std::snprintf(line, sizeof(line), "eeprom %s in %lu vbls, word0 %04X",
                valid && after == bank ? "ok" : "FAIL",
                static_cast<unsigned long>(runtime::vblCount() - start),
                after[0]);
  report(line);
  if (valid && after == bank) {
    ++passes;
  } else {
    ++failures;
  }
}

} // namespace

int main() {
  runtime::installVectors();
  console::attach("OpenFranko Jaguar self test");
  profiler::start();
  const Case cases[] = {
      {"copy 320x200", Kind::Copy, 0, 0, 320, 200, 320, 320, true},
      {"copy aligned 37", Kind::Copy, 0, 0, 37, 4, 64, 64, false},
      {"copy dst+3", Kind::Copy, 0, 3, 37, 4, 64, 64, false},
      {"copy src+5", Kind::Copy, 5, 0, 37, 4, 64, 64, false},
      {"copy src6 dst1", Kind::Copy, 6, 1, 50, 3, 72, 80, false},
      {"copy src1 dst6", Kind::Copy, 1, 6, 50, 3, 80, 72, false},
      {"copy src7 w1", Kind::Copy, 7, 0, 1, 3, 64, 64, false},
      {"copy 8x3000", Kind::Copy, 0, 0, 8, 3000, 8, 8, false},
      {"copy 1008 src+3", Kind::Copy, 3, 0, 300, 120, 1008, 368, false},
      {"copy p37 short", Kind::Copy, 0, 3, 37, 5, 37, 64, false},
      {"copy p37 tall", Kind::Copy, 0, 3, 37, 40, 37, 64, false},
      {"copy p36 tall", Kind::Copy, 2, 5, 30, 33, 36, 64, false},
      {"copy p37 neg", Kind::Copy, 0, 1, 37, 40, -37, 64, false},
      {"copy p64 neg", Kind::Copy, 3, 1, 50, 40, -64, 64, false},
      {"copy p365 big", Kind::Copy, 0, 0, 365, 290, 365, 368, true},
      {"mask src2 dst5", Kind::Masked, 2, 5, 30, 4, 64, 64, false},
      {"mask src5 dst2", Kind::Masked, 5, 2, 33, 5, 64, 72, false},
      {"mask p37 tall", Kind::Masked, 0, 3, 37, 40, 37, 64, false},
      {"mask p41 neg", Kind::Masked, 0, 6, 41, 33, -41, 64, false},
      {"mask p48 bob", Kind::Masked, 0, 3, 48, 64, 48, 368, true},
      {"mask p37 bob", Kind::Masked, 0, 3, 37, 64, 37, 368, true},
      {"mirror p37", Kind::Mirrored, 0, 3, 37, 9, 37, 64, false},
      {"mirror p64 neg", Kind::Mirrored, 1, 2, 40, 9, -64, 64, false},
      {"mirmask p37", Kind::MirroredMasked, 0, 5, 37, 9, 37, 64, false},
      {"mirmask p37 neg", Kind::MirroredMasked, 0, 5, 37, 21, -37, 64, false},
      {"mirmask p37 bob", Kind::MirroredMasked, 0, 3, 37, 64, 37, 368, true},
      {"mirror 320x200", Kind::Mirrored, 0, 0, 320, 200, 320, 320, true},
      {"copy w6 dst5", Kind::Copy, 1, 5, 6, 7, 64, 72, false},
      {"mask w5", Kind::Masked, 3, 1, 5, 6, 64, 64, false},
      {"mask w1 neg", Kind::Masked, 2, 7, 1, 9, -64, 64, false},
      {"mirror w7 neg", Kind::Mirrored, 1, 4, 7, 6, -64, 64, false},
      {"mirmask w3", Kind::MirroredMasked, 2, 6, 3, 5, 64, 64, false},
      {"mirmask w2 p37", Kind::MirroredMasked, 0, 3, 2, 8, 37, 64, false},
  };
  const auto runBlits = [&cases]() {
    for (const Case &test : cases) {
      runCopy(test);
    }
    runFill("fill dst+3", 3, 37, 4, 64);
    runFill("fill aligned", 0, 64, 3, 64);
    runFill("fill p37", 2, 30, 9, 37);
    runFill("fill p37 tall", 5, 30, 40, 37);
    runFill("fill p322", 1, 300, 200, 322);
    runFill("fill w3", 6, 3, 5, 64);
    runFill("fill w1 p37", 4, 1, 7, 37);
  };
  runBlits();
  const RiscProgram gpu = gpuProgram();
  longWord(GPU_CTRL) = 0;
  loadProgram(gpu);
  blitter::useQueue(gpu.entries[2]);
  longWord(GPU_PC) = gpu.entries[0];
  longWord(GPU_CTRL) = RISC_GO;
  report("GPU queue:");
  runBlits();
  queueCalls = true;
  for (const Case &test : cases) {
    runCopy(test);
  }
  queueCalls = false;
  testLz4();
  testOutline();
  testFlip();
  testTranslate();
  testMemory();
  blitter::stopQueue();
  testEeprom();
  char summary[64];
  std::snprintf(summary, sizeof(summary), "Self test: %d passed, %d failed",
                passes, failures);
  report(summary);
  console::show(summary, failures != 0);
}
