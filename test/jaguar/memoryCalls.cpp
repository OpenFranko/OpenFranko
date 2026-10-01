#include "memoryCalls.h"

#include <cstddef>
#include <cstdint>

extern "C" {
void *__real_memcpy(void *target, const void *source, std::size_t size);
void *__real_memmove(void *target, const void *source, std::size_t size);
void *__real_memset(void *target, int value, std::size_t size);
}

namespace memory_calls {
namespace {

constexpr std::size_t SLOTS = 512;

CallSite sites[SLOTS];
bool recording = false;

bool inInterrupt() {
  uint16_t status = 0;
  asm volatile("move.w %%sr,%0" : "=d"(status));
  return (status & 0x0700) != 0;
}

void note(uint32_t caller, std::size_t bytes, uint8_t kind) {
  if (!recording || inInterrupt()) {
    return;
  }
  std::size_t slot = (caller >> 1) % SLOTS;
  for (std::size_t probe = 0; probe < SLOTS; ++probe) {
    CallSite &site = sites[slot];
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

} // namespace

void start() {
  for (CallSite &site : sites) {
    site = CallSite{};
  }
  recording = true;
}

void stop() { recording = false; }

std::vector<CallSite> heaviest(std::size_t count) {
  std::vector<CallSite> found;
  for (const CallSite &site : sites) {
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

} // namespace memory_calls

extern "C" {

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
}
