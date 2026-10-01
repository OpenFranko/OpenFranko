#ifndef SYSTEMS_JAGUAR_PROFILER_H_
#define SYSTEMS_JAGUAR_PROFILER_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {
namespace profiler {

inline constexpr int TICK_MICROSECONDS = 10;

void start();
uint16_t now();
uint16_t since(uint16_t earlier);

} // namespace profiler
} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_PROFILER_H_
