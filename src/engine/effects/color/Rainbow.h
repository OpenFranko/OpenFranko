#ifndef ENGINE_EFFECTS_RAINBOW_H_
#define ENGINE_EFFECTS_RAINBOW_H_

#include "AmigaPalette.h"

#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

AmigaPalette rainbowTable(int height, const std::string &red,
                          const std::string &green, const std::string &blue,
                          AmigaColor start = 0);

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_RAINBOW_H_
