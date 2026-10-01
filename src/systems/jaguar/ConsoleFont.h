#ifndef SYSTEMS_JAGUAR_CONSOLEFONT_H_
#define SYSTEMS_JAGUAR_CONSOLEFONT_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {

inline constexpr int GLYPH_WIDTH = 5;
inline constexpr int GLYPH_HEIGHT = 7;
inline constexpr int CELL_WIDTH = 6;
inline constexpr int CELL_HEIGHT = 8;
inline constexpr unsigned char FIRST_GLYPH = 32;
inline constexpr int GLYPH_COUNT = 98;
inline constexpr unsigned char BACKSPACE_GLYPH = 127;
inline constexpr unsigned char RETURN_GLYPH = 128;
inline constexpr unsigned char SPACE_GLYPH = 129;

const uint8_t *glyph(unsigned char code);

} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_CONSOLEFONT_H_
