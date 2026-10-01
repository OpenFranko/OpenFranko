#include "Blitter.h"

#include <cstddef>
#include <cstdint>

extern "C" {
void jaguarCopyForward(void *target, const void *source, std::size_t size);
void jaguarCopyBackward(void *target, const void *source, std::size_t size);
void jaguarFill(void *target, int value, std::size_t size);
}

namespace openfranko::src::systems::jaguar {
namespace {

constexpr std::size_t BLITTER_BYTES = 1024;
constexpr int ROW_BYTES = 2048;
constexpr uintptr_t DRAM_END = 0x200000;
constexpr uint16_t INTERRUPT_LEVEL = 0x0700;

bool inInterrupt() {
  uint16_t status = 0;
  asm volatile("move.w %%sr,%0" : "=d"(status));
  return (status & INTERRUPT_LEVEL) != 0;
}

bool inDram(const void *pointer, std::size_t size) {
  return reinterpret_cast<uintptr_t>(pointer) + size <= DRAM_END;
}

bool blittable(void *target, const void *source, std::size_t size) {
  return size >= BLITTER_BYTES && inDram(target, size) &&
         (!source || inDram(source, size)) && !inInterrupt();
}

void blitCopy(void *target, const void *source, std::size_t size) {
  uint8_t *to = static_cast<uint8_t *>(target);
  const uint8_t *from = static_cast<const uint8_t *>(source);
  const int rows = static_cast<int>(size / ROW_BYTES);
  const std::size_t body = static_cast<std::size_t>(rows) * ROW_BYTES;
  if (rows > 0) {
    blitter::copy({from, ROW_BYTES}, {to, ROW_BYTES}, ROW_BYTES, rows);
    blitter::wait();
  }
  if (size > body) {
    jaguarCopyForward(to + body, from + body, size - body);
  }
}

void blitFill(void *target, int value, std::size_t size) {
  uint8_t *to = static_cast<uint8_t *>(target);
  const int rows = static_cast<int>(size / ROW_BYTES);
  const std::size_t body = static_cast<std::size_t>(rows) * ROW_BYTES;
  if (rows > 0) {
    blitter::fill({to, ROW_BYTES}, ROW_BYTES, rows,
                  static_cast<uint8_t>(value));
    blitter::wait();
  }
  if (size > body) {
    jaguarFill(to + body, value, size - body);
  }
}

} // namespace
} // namespace openfranko::src::systems::jaguar

using namespace openfranko::src::systems::jaguar;

extern "C" {

void *memcpy(void *target, const void *source, std::size_t size) {
  if (blittable(target, source, size)) {
    blitCopy(target, source, size);
  } else {
    jaguarCopyForward(target, source, size);
  }
  return target;
}

void *memmove(void *target, const void *source, std::size_t size) {
  const uint8_t *from = static_cast<const uint8_t *>(source);
  uint8_t *to = static_cast<uint8_t *>(target);
  if (to >= from + size || from >= to + size) {
    return memcpy(target, source, size);
  }
  if (to < from) {
    jaguarCopyForward(target, source, size);
  } else if (to > from) {
    jaguarCopyBackward(target, source, size);
  }
  return target;
}

int memcmp(const void *left, const void *right, std::size_t size) {
  const uint8_t *first = static_cast<const uint8_t *>(left);
  const uint8_t *second = static_cast<const uint8_t *>(right);
  if (((reinterpret_cast<uintptr_t>(first) ^
        reinterpret_cast<uintptr_t>(second)) &
       1) == 0) {
    if ((reinterpret_cast<uintptr_t>(first) & 1) != 0 && size != 0) {
      if (*first != *second) {
        return *first - *second;
      }
      ++first;
      ++second;
      --size;
    }
    while (size >= 4 && *reinterpret_cast<const uint32_t *>(first) ==
                            *reinterpret_cast<const uint32_t *>(second)) {
      first += 4;
      second += 4;
      size -= 4;
    }
  }
  for (; size != 0; --size, ++first, ++second) {
    if (*first != *second) {
      return *first - *second;
    }
  }
  return 0;
}

void *memset(void *target, int value, std::size_t size) {
  if (blittable(target, nullptr, size)) {
    blitFill(target, value, size);
  } else {
    jaguarFill(target, value, size);
  }
  return target;
}
}
