#ifndef ENGINE_EFFECTS_ANIMATION_BOB_H_
#define ENGINE_EFFECTS_ANIMATION_BOB_H_

#include <cstdint>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {
namespace animation {

struct Bob {
  bool shown = false;
  int16_t x = 0;
  int16_t y = 0;
  int image = 0;
  bool flipped = false;
};

} // namespace animation
} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_ANIMATION_BOB_H_
