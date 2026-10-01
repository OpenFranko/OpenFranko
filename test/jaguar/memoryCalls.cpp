#include "memoryCalls.h"

#include <cstddef>
#include <cstdint>

extern "C" {
#ifdef PROFILE_MEMORY
void *__real_memcpy(void *target, const void *source, std::size_t size);
void *__real_memmove(void *target, const void *source, std::size_t size);
void *__real_memset(void *target, int value, std::size_t size);
#endif
#ifdef PROFILE_ARITHMETIC
long __real___mulsi3(long left, long right);
unsigned long __real___udivsi3(unsigned long left, unsigned long right);
long __real___divsi3(long left, long right);
unsigned long __real___umodsi3(unsigned long left, unsigned long right);
long __real___modsi3(long left, long right);
long long __real___muldi3(long long left, long long right);
#endif
}

namespace memory_calls {
namespace {

constexpr std::size_t SLOTS = 512;

CallSite sites[SLOTS];
CallSite arithmetic[SLOTS];
bool recording = false;

bool inInterrupt() {
  uint16_t status = 0;
  asm volatile("move.w %%sr,%0" : "=d"(status));
  return (status & 0x0700) != 0;
}

void record(CallSite *table, uint32_t caller, std::size_t bytes, uint8_t kind) {
  if (!recording || inInterrupt()) {
    return;
  }
  std::size_t slot = (caller >> 1) % SLOTS;
  for (std::size_t probe = 0; probe < SLOTS; ++probe) {
    CallSite &site = table[slot];
    if (site.caller == caller || site.caller == 0) {
      site.caller = caller;
      site.kind = kind;
      ++site.calls;
      site.bytes += bytes;
      return;
    }
    slot = (slot + 1) % SLOTS;
  }
}

void note(uint32_t caller, std::size_t bytes, uint8_t kind) {
  record(sites, caller, bytes, kind);
}

void noteArithmetic(uint32_t caller, uint8_t kind) {
  record(arithmetic, caller, 1, kind);
}

std::vector<CallSite> sorted(const CallSite *table, std::size_t count) {
  std::vector<CallSite> found;
  for (std::size_t index = 0; index < SLOTS; ++index) {
    const CallSite &site = table[index];
    if (site.caller == 0) {
      continue;
    }
    std::size_t at = found.size();
    found.push_back(site);
    while (at > 0 && found[at - 1].bytes < found[at].bytes) {
      const CallSite swap = found[at - 1];
      found[at - 1] = found[at];
      found[at] = swap;
      --at;
    }
    if (found.size() > count) {
      found.pop_back();
    }
  }
  return found;
}

} // namespace

void start() {
  for (CallSite &site : sites) {
    site = CallSite{};
  }
  for (CallSite &site : arithmetic) {
    site = CallSite{};
  }
  recording = true;
}

void stop() { recording = false; }

std::vector<CallSite> heaviest(std::size_t count) {
  return sorted(sites, count);
}

std::vector<CallSite> busiestArithmetic(std::size_t count) {
  return sorted(arithmetic, count);
}

} // namespace memory_calls

extern "C" {

#ifdef PROFILE_MEMORY
void *__wrap_memcpy(void *target, const void *source, std::size_t size) {
  memory_calls::note(reinterpret_cast<uint32_t>(__builtin_return_address(0)),
                     size, 'c');
  return __real_memcpy(target, source, size);
}

void *__wrap_memmove(void *target, const void *source, std::size_t size) {
  memory_calls::note(reinterpret_cast<uint32_t>(__builtin_return_address(0)),
                     size, 'm');
  return __real_memmove(target, source, size);
}

void *__wrap_memset(void *target, int value, std::size_t size) {
  memory_calls::note(reinterpret_cast<uint32_t>(__builtin_return_address(0)),
                     size, 's');
  return __real_memset(target, value, size);
}
#endif

#ifdef PROFILE_ARITHMETIC
long __wrap___mulsi3(long left, long right) {
  memory_calls::noteArithmetic(
      reinterpret_cast<uint32_t>(__builtin_return_address(0)), 'M');
  return __real___mulsi3(left, right);
}

unsigned long __wrap___udivsi3(unsigned long left, unsigned long right) {
  memory_calls::noteArithmetic(
      reinterpret_cast<uint32_t>(__builtin_return_address(0)), 'D');
  return __real___udivsi3(left, right);
}

long __wrap___divsi3(long left, long right) {
  memory_calls::noteArithmetic(
      reinterpret_cast<uint32_t>(__builtin_return_address(0)), 'd');
  return __real___divsi3(left, right);
}

unsigned long __wrap___umodsi3(unsigned long left, unsigned long right) {
  memory_calls::noteArithmetic(
      reinterpret_cast<uint32_t>(__builtin_return_address(0)), 'U');
  return __real___umodsi3(left, right);
}

long __wrap___modsi3(long left, long right) {
  memory_calls::noteArithmetic(
      reinterpret_cast<uint32_t>(__builtin_return_address(0)), 'u');
  return __real___modsi3(left, right);
}

long long __wrap___muldi3(long long left, long long right) {
  memory_calls::noteArithmetic(
      reinterpret_cast<uint32_t>(__builtin_return_address(0)), '6');
  return __real___muldi3(left, right);
}
#endif
}
