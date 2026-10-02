#include "MersenneTwister.h"

#include "../systems/Multiply.h"

namespace openfranko::src::engine {
namespace {

constexpr std::size_t SHIFT_SIZE = 397;
constexpr uint32_t SEED_FACTOR = 1812433253;
constexpr int SEED_SHIFT = 30;
constexpr uint32_t UPPER_MASK = 0x80000000;
constexpr uint32_t LOWER_MASK = 0x7FFFFFFF;
constexpr uint32_t TWIST_MATRIX = 0x9908B0DF;
constexpr int TEMPER_SHIFT_U = 11;
constexpr int TEMPER_SHIFT_S = 7;
constexpr uint32_t TEMPER_MASK_B = 0x9D2C5680;
constexpr int TEMPER_SHIFT_T = 15;
constexpr uint32_t TEMPER_MASK_C = 0xEFC60000;
constexpr int TEMPER_SHIFT_L = 18;
constexpr int HALF_BITS = 16;
constexpr uint32_t HALF_MASK = 0xFFFF;

struct Product {
  uint32_t high = 0;
  uint32_t low = 0;
};

Product multiplyWide(uint32_t left, uint32_t right) {
  const uint16_t leftLow = static_cast<uint16_t>(left & HALF_MASK);
  const uint16_t leftHigh = static_cast<uint16_t>(left >> HALF_BITS);
  const uint16_t rightLow = static_cast<uint16_t>(right & HALF_MASK);
  const uint16_t rightHigh = static_cast<uint16_t>(right >> HALF_BITS);
  const uint32_t lowest = systems::multiplyUnsigned16(leftLow, rightLow);
  const uint32_t first = systems::multiplyUnsigned16(leftHigh, rightLow);
  const uint32_t second = systems::multiplyUnsigned16(leftLow, rightHigh);
  Product product;
  product.high = systems::multiplyUnsigned16(leftHigh, rightHigh) +
                 (first >> HALF_BITS) + (second >> HALF_BITS);
  product.low = lowest + (first << HALF_BITS);
  product.high += product.low < lowest ? 1 : 0;
  const uint32_t before = product.low;
  product.low += second << HALF_BITS;
  product.high += product.low < before ? 1 : 0;
  return product;
}

} // namespace

MersenneTwister::MersenneTwister(result_type seed) {
  m_state[0] = seed;
  for (std::size_t i = 1; i < STATE_SIZE; ++i) {
    const uint32_t previous = m_state[i - 1];
    m_state[i] = SEED_FACTOR * (previous ^ (previous >> SEED_SHIFT)) +
                 static_cast<uint32_t>(i);
  }
}

MersenneTwister::result_type MersenneTwister::operator()() {
  const std::size_t next = m_index + 1 < STATE_SIZE ? m_index + 1 : 0;
  const std::size_t shifted = m_index < STATE_SIZE - SHIFT_SIZE
                                  ? m_index + SHIFT_SIZE
                                  : m_index - (STATE_SIZE - SHIFT_SIZE);
  const uint32_t mixed =
      (m_state[m_index] & UPPER_MASK) | (m_state[next] & LOWER_MASK);
  uint32_t value =
      m_state[shifted] ^ (mixed >> 1) ^ ((mixed & 1) != 0 ? TWIST_MATRIX : 0);
  m_state[m_index] = value;
  m_index = next;
  value ^= value >> TEMPER_SHIFT_U;
  value ^= (value << TEMPER_SHIFT_S) & TEMPER_MASK_B;
  value ^= (value << TEMPER_SHIFT_T) & TEMPER_MASK_C;
  value ^= value >> TEMPER_SHIFT_L;
  return value;
}

MersenneTwister::result_type MersenneTwister::upTo(result_type limit) {
  const uint32_t range = limit + 1;
  if (range == 0) {
    return (*this)();
  }
  Product product = multiplyWide((*this)(), range);
  if (product.low < range) {
    const uint32_t threshold = (0u - range) % range;
    while (product.low < threshold) {
      product = multiplyWide((*this)(), range);
    }
  }
  return product.high;
}

} // namespace openfranko::src::engine
