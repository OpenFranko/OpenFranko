#ifndef ENGINE_EFFECTS_AMIGAPALETTE_H_
#define ENGINE_EFFECTS_AMIGAPALETTE_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

using AmigaColor = uint16_t;
using AmigaPalette = std::vector<AmigaColor>;

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_AMIGAPALETTE_H_
