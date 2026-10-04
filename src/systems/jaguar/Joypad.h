#ifndef SYSTEMS_JAGUAR_JOYPAD_H_
#define SYSTEMS_JAGUAR_JOYPAD_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {

inline constexpr uint32_t PAD_HASH = 1u << 0;
inline constexpr uint32_t PAD_9 = 1u << 1;
inline constexpr uint32_t PAD_6 = 1u << 2;
inline constexpr uint32_t PAD_3 = 1u << 3;
inline constexpr uint32_t PAD_0 = 1u << 4;
inline constexpr uint32_t PAD_8 = 1u << 5;
inline constexpr uint32_t PAD_5 = 1u << 6;
inline constexpr uint32_t PAD_2 = 1u << 7;
inline constexpr uint32_t PAD_OPTION = 1u << 9;
inline constexpr uint32_t PAD_C = 1u << 13;
inline constexpr uint32_t PAD_STAR = 1u << 16;
inline constexpr uint32_t PAD_7 = 1u << 17;
inline constexpr uint32_t PAD_4 = 1u << 18;
inline constexpr uint32_t PAD_1 = 1u << 19;
inline constexpr uint32_t PAD_UP = 1u << 20;
inline constexpr uint32_t PAD_DOWN = 1u << 21;
inline constexpr uint32_t PAD_LEFT = 1u << 22;
inline constexpr uint32_t PAD_RIGHT = 1u << 23;
inline constexpr uint32_t PAD_B = 1u << 25;
inline constexpr uint32_t PAD_PAUSE = 1u << 28;
inline constexpr uint32_t PAD_A = 1u << 29;

uint32_t readJoypad();

} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_JOYPAD_H_
