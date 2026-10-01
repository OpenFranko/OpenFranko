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
  testMemory();
  blitter::stopQueue();
  testEeprom();
  char summary[64];
  std::snprintf(summary, sizeof(summary), "Self test: %d passed, %d failed",
                passes, failures);
  report(summary);
  console::show(summary, failures != 0);
}
