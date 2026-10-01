#include "Joypad.h"

#include "Hardware.h"

namespace openfranko::src::systems::jaguar {
namespace {

constexpr uint32_t UNUSED_BITS = 0xF0FFFFFCu;
constexpr uint16_t ROW_SELECT[4] = {0x81FE, 0x81FD, 0x81FB, 0x81F7};

uint32_t rotateRight(uint32_t value, int bits) {
  return value >> bits | value << (32 - bits);
}

uint32_t rotateLeft(uint32_t value, int bits) {
  return value << bits | value >> (32 - bits);
}

uint32_t readRow(int row) {
  word(JOYSTICK) = ROW_SELECT[row];
  return longWord(JOYSTICK) | UNUSED_BITS;
}

} // namespace

uint32_t readJoypad() {
  uint32_t active = ~0u;
  active &= rotateRight(readRow(0), 4);
  active &= rotateRight(readRow(1), 8);
  active &= rotateLeft(readRow(2), 12);
  active &= rotateLeft(readRow(3), 8);
  return ~active;
}

} // namespace openfranko::src::systems::jaguar
