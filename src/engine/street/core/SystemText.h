#ifndef ENGINE_STREET_CORE_SYSTEMTEXT_H_
#define ENGINE_STREET_CORE_SYSTEMTEXT_H_

#include "IndexedSurface.h"

#include <cstdint>
#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace core {

inline constexpr int SYSTEM_FONT_WIDTH = 8;
inline constexpr int SYSTEM_FONT_HEIGHT = 8;
inline constexpr int SYSTEM_FONT_BASELINE = 6;

void drawSystemText(IndexedSurface &surface, int x, int baseline,
                    const std::string &text, uint8_t ink, uint8_t paper);

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CORE_SYSTEMTEXT_H_
