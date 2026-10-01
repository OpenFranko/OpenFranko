#ifndef SYSTEMS_JAGUAR_VIRTUALKEYBOARD_H_
#define SYSTEMS_JAGUAR_VIRTUALKEYBOARD_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {
namespace keyboard {

inline constexpr int WIDTH = 216;
inline constexpr int LINES = 2;
inline constexpr int HEIGHT = LINES * 8;

void setOpen(bool open);
bool isOpen();
void move(int steps);
char selected();
const uint16_t *pixels();

} // namespace keyboard
} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_VIRTUALKEYBOARD_H_
