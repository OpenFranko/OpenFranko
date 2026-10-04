#include "sampler.h"

#include "../../src/systems/jaguar/Runtime.h"

#include <algorithm>
#include <array>

namespace sampler {
namespace {

namespace runtime = openfranko::src::systems::jaguar::runtime;

constexpr int BUCKET_SHIFT = 4;
constexpr std::size_t BUCKETS = 0x10000;
constexpr uint16_t SATURATED = 0xFFFF;
constexpr uint16_t PRESCALER = 26;
constexpr uint16_t DIVIDER = 245;

constexpr std::size_t FRAME_SAMPLES = 1024;

std::array<uint16_t, BUCKETS> histogram;
uint32_t first = 0;
uint32_t last = 0;
volatile uint32_t sampled = 0;
volatile uint32_t missed = 0;
bool framed = false;
std::array<uint32_t, FRAME_SAMPLES> frameSamples;
volatile std::size_t frameCount = 0;

void count(uint32_t pc) {
  sampled = sampled + 1;
  if (pc < first || pc >= last) {
    missed = missed + 1;
    return;
  }
  uint16_t &bucket = histogram[(pc - first) >> BUCKET_SHIFT];
  if (bucket != SATURATED) {
    ++bucket;
  }
}

void record(uint32_t pc) {
  if (!framed) {
    count(pc);
    return;
  }
  const std::size_t at = frameCount;
  if (at < FRAME_SAMPLES) {
    frameSamples[at] = pc;
    frameCount = at + 1;
  }
}

} // namespace

void start(uint32_t codeStart, uint32_t codeEnd) {
  histogram.fill(0);
  first = codeStart;
  last = std::min(codeEnd, codeStart + (BUCKETS << BUCKET_SHIFT));
  sampled = 0;
  missed = 0;
  runtime::startTimer(PRESCALER, DIVIDER, record);
}

void stop() {
  runtime::stopTimer();
  framed = false;
}

void beginFrame() {
  framed = true;
  frameCount = 0;
}

void endFrame(bool keep) {
  const std::size_t samples = frameCount;
  frameCount = 0;
  if (keep) {
    for (std::size_t index = 0; index < samples; ++index) {
      count(frameSamples[index]);
    }
  }
}

uint32_t total() { return sampled; }

uint32_t outside() { return missed; }

std::vector<Hotspot> hottest(std::size_t count) {
  std::vector<Hotspot> spots;
  for (std::size_t bucket = 0; bucket < BUCKETS; ++bucket) {
    if (histogram[bucket] != 0) {
      spots.push_back({first + static_cast<uint32_t>(bucket << BUCKET_SHIFT),
                       histogram[bucket]});
    }
  }
  const std::size_t shown = std::min(count, spots.size());
  std::partial_sort(spots.begin(),
                    spots.begin() + static_cast<std::ptrdiff_t>(shown),
                    spots.end(), [](const Hotspot &left, const Hotspot &right) {
                      return left.samples > right.samples;
                    });
  spots.resize(shown);
  return spots;
}

} // namespace sampler
