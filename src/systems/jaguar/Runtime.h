#ifndef SYSTEMS_JAGUAR_RUNTIME_H_
#define SYSTEMS_JAGUAR_RUNTIME_H_

#include <cstddef>
#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {
namespace runtime {

using InterruptHandler = void (*)();
using TimerHandler = void (*)(uint32_t interruptedPc);

void installVectors();
void setVideoHandler(InterruptHandler handler);
void enableVideoInterrupt(int halfLine);
void startTimer(uint16_t prescaler, uint16_t divider, TimerHandler handler);
void stopTimer();
void armWatchdog(uint32_t vbls);
void feedWatchdog();
std::size_t heapUsed();
std::size_t heapPeak();
std::size_t heapCapacity();
uint32_t romEnd();
uint32_t vblCount();

} // namespace runtime
} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_RUNTIME_H_
