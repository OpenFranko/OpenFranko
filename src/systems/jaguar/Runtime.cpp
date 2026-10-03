#include "Runtime.h"

#include "Console.h"
#include "Hardware.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>

extern "C" {
extern char __heap_start[];
extern char __heap_end[];
extern char __rom_end[];
void jaguarLevel0Interrupt();
extern char jaguarExceptionStubs[];
}

namespace openfranko::src::systems::jaguar::runtime {
namespace {

constexpr int FIRST_EXCEPTION = 2;
constexpr int LAST_EXCEPTION = 63;
constexpr int STUB_BYTES = 16;
constexpr int VECTOR_BYTES = 4;
constexpr int SAVED_REGISTERS = 16;
constexpr int ADDRESS_ERROR = 3;
constexpr int STDOUT = 1;
constexpr int STDERR = 2;

char *heapTop = __heap_start;
std::size_t peak = 0;
InterruptHandler videoHandler = nullptr;
TimerHandler timerHandler = nullptr;
volatile uint32_t vbls = 0;
volatile uint16_t enabledSources = 0;
uint32_t watchdogLimit = 0;
volatile uint32_t watchdogFed = 0;

const char *exceptionName(uint32_t vector) {
  switch (vector) {
  case 2:
    return "bus error";
  case 3:
    return "address error";
  case 4:
    return "illegal instruction";
  case 5:
    return "division by zero";
  case 6:
    return "CHK";
  case 7:
    return "TRAPV";
  case 8:
    return "privilege violation";
  case 9:
    return "trace";
  case 10:
  case 11:
    return "unimplemented instruction";
  case 15:
    return "uninitialized interrupt";
  case 24:
    return "spurious interrupt";
  default:
    if (vector >= 25 && vector <= 31) {
      return "autovector interrupt";
    }
    if (vector >= 32 && vector <= 47) {
      return "TRAP";
    }
    return "reserved exception";
  }
}

uint32_t readLong(const uint8_t *at) {
  return static_cast<uint32_t>(at[0]) << 24 |
         static_cast<uint32_t>(at[1]) << 16 |
         static_cast<uint32_t>(at[2]) << 8 | at[3];
}

} // namespace

void installVectors() {
  for (int vector = FIRST_EXCEPTION; vector <= LAST_EXCEPTION; ++vector) {
    port<uint32_t>(static_cast<uint32_t>(vector * VECTOR_BYTES)) =
        reinterpret_cast<uint32_t>(jaguarExceptionStubs) +
        static_cast<uint32_t>((vector - FIRST_EXCEPTION) * STUB_BYTES);
  }
  port<uint32_t>(LEVEL0_VECTOR) =
      reinterpret_cast<uint32_t>(&jaguarLevel0Interrupt);
}

void setVideoHandler(InterruptHandler handler) { videoHandler = handler; }

void enableVideoInterrupt(int halfLine) {
  word(VI) = static_cast<uint16_t>(halfLine | 1);
  enabledSources = enabledSources | INT1_VIDEO;
  word(INT1) = enabledSources;
  asm volatile("move.w #0x2000,%%sr" ::: "cc");
}

void startTimer(uint16_t prescaler, uint16_t divider, TimerHandler handler) {
  timerHandler = handler;
  word(PIT0) = prescaler;
  word(PIT1) = divider;
  enabledSources = enabledSources | INT1_TIMER;
  word(INT1) = enabledSources;
}

void armWatchdog(uint32_t limit) {
  watchdogFed = vbls;
  watchdogLimit = limit;
}

void feedWatchdog() { watchdogFed = vbls; }

void stopTimer() {
  enabledSources = enabledSources & static_cast<uint16_t>(~INT1_TIMER);
  word(INT1) = enabledSources;
  word(PIT0) = 0;
  word(PIT1) = 0;
  timerHandler = nullptr;
}

std::size_t heapUsed() {
  return static_cast<std::size_t>(heapTop - __heap_start);
}

std::size_t heapPeak() { return peak; }

std::size_t heapCapacity() {
  return static_cast<std::size_t>(__heap_end - __heap_start);
}

uint32_t romEnd() { return reinterpret_cast<uint32_t>(__rom_end); }

uint32_t vblCount() { return vbls; }

} // namespace openfranko::src::systems::jaguar::runtime

using namespace openfranko::src::systems::jaguar;

extern "C" {

void jaguarDispatchInterrupt(uint32_t pending, uint32_t interruptedPc) {
  if (pending & INT1_TIMER) {
    if (runtime::timerHandler) {
      runtime::timerHandler(interruptedPc);
    }
  }
  if (pending & INT1_VIDEO) {
    runtime::vbls = runtime::vbls + 1;
    if (runtime::videoHandler) {
      runtime::videoHandler();
    }
    if (runtime::watchdogLimit != 0 &&
        runtime::vbls - runtime::watchdogFed > runtime::watchdogLimit) {
      char message[80];
      std::snprintf(message, sizeof(message),
                    "Watchdog: stuck at PC %06lX, blitter %08lX",
                    static_cast<unsigned long>(interruptedPc),
                    static_cast<unsigned long>(longWord(B_CMD)));
      console::show(message, true);
    }
  }
  word(INT1) = static_cast<uint16_t>(runtime::enabledSources |
                                     (pending & 0x1F) << INT1_CLEAR_SHIFT);
  word(INT2) = 0;
}

void jaguarFatalException(uint32_t *saved) {
  const uint32_t vector = saved[runtime::SAVED_REGISTERS];
  const uint8_t *frame =
      reinterpret_cast<const uint8_t *>(saved + runtime::SAVED_REGISTERS + 1);
  char message[160];
  if (vector <= runtime::ADDRESS_ERROR) {
    std::snprintf(message, sizeof(message), "CPU %s at PC %06lX, address %06lX",
                  runtime::exceptionName(vector),
                  static_cast<unsigned long>(runtime::readLong(frame + 10)),
                  static_cast<unsigned long>(runtime::readLong(frame + 2)));
  } else {
    std::snprintf(message, sizeof(message), "CPU %s (%lu) at PC %06lX",
                  runtime::exceptionName(vector),
                  static_cast<unsigned long>(vector),
                  static_cast<unsigned long>(runtime::readLong(frame + 2)));
  }
  console::show(message, true);
}

void *sbrk(ptrdiff_t increment) {
  if (increment > __heap_end - runtime::heapTop ||
      increment < __heap_start - runtime::heapTop) {
    errno = ENOMEM;
    return reinterpret_cast<void *>(-1);
  }
  char *previous = runtime::heapTop;
  runtime::heapTop += increment;
  if (runtime::heapUsed() > runtime::peak) {
    runtime::peak = runtime::heapUsed();
  }
  return previous;
}

int write(int file, const void *data, unsigned size) {
  if (file == runtime::STDOUT || file == runtime::STDERR) {
    console::write(static_cast<const char *>(data), size);
  }
  return static_cast<int>(size);
}

int read(int, void *, unsigned) { return 0; }

int open(const char *, int, ...) {
  errno = ENOENT;
  return -1;
}

int close(int) { return -1; }

int lseek(int, int, int) { return -1; }

int fstat(int, struct stat *status) {
  std::memset(status, 0, sizeof(*status));
  status->st_mode = S_IFCHR;
  return 0;
}

int isatty(int) { return 1; }

int kill(int, int) {
  errno = EINVAL;
  return -1;
}

int getpid() { return 1; }

int getentropy(void *buffer, size_t size) {
  static uint32_t state = 0x6A09E667u;
  uint8_t *out = static_cast<uint8_t *>(buffer);
  for (size_t at = 0; at < size; ++at) {
    state ^= static_cast<uint32_t>(word(HC)) << 16 | word(VC);
    state ^= runtime::vbls * 0x9E3779B9u;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    out[at] = static_cast<uint8_t>(state >> 24);
  }
  return 0;
}

void _exit(int code) {
  console::show(code == 0 ? "OpenFranko has finished"
                          : "OpenFranko has stopped",
                code != 0);
}
}
