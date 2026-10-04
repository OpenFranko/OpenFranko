#include "Profiler.h"

#include "Hardware.h"

namespace openfranko::src::systems::jaguar::profiler {
namespace {

constexpr uint16_t PRESCALER = 265;
constexpr uint16_t DIVIDER = 0xFFFF;
constexpr uint32_t TIMER2_PRESCALER_READ = JERRY + 0x003A;
constexpr uint32_t TIMER2_DIVIDER_READ = JERRY + 0x003C;

uint32_t busyTicks = 0;

} // namespace

void start() {
  word(JPIT3) = PRESCALER;
  word(JPIT4) = DIVIDER;
}

uint16_t now() {
  return static_cast<uint16_t>(DIVIDER - word(TIMER2_DIVIDER_READ));
}

uint16_t since(uint16_t earlier) {
  return static_cast<uint16_t>(now() - earlier);
}

void addBusy(uint16_t ticks) { busyTicks += ticks; }

uint32_t busy() { return busyTicks; }

} // namespace openfranko::src::systems::jaguar::profiler
