#ifndef SYSTEMS_JAGUAR_DEBUGOVERLAY_H_
#define SYSTEMS_JAGUAR_DEBUGOVERLAY_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {
namespace overlay {

inline constexpr int WIDTH = 192;
inline constexpr int LINES = 2;
inline constexpr int HEIGHT = LINES * 8;

void setEnabled(bool enabled);
bool isEnabled();
void setLine(int line, const char *text);
const uint16_t *pixels();

} // namespace overlay
} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_DEBUGOVERLAY_H_
