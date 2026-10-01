#ifndef SYSTEMS_MULTIPLY_H_
#define SYSTEMS_MULTIPLY_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {

inline int32_t multiplySigned16(int16_t left, int16_t right) {
#if defined(__mc68000__)
  int32_t result = left;
  asm("muls.w %1,%0" : "+d"(result) : "d"(right) : "cc");
  return result;
#else
  return static_cast<int32_t>(left) * right;
#endif
}

inline uint32_t multiplyUnsigned16(uint16_t left, uint16_t right) {
#if defined(__mc68000__)
  uint32_t result = left;
  asm("mulu.w %1,%0" : "+d"(result) : "d"(right) : "cc");
  return result;
#else
  return static_cast<uint32_t>(left) * right;
#endif
}

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_MULTIPLY_H_
