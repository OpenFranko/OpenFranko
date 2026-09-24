#ifndef ENGINE_EFFECTS_PALETTEFLASHER_H_
#define ENGINE_EFFECTS_PALETTEFLASHER_H_

#include "AmigaPalette.h"

#include <cstddef>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

struct FlashStep {
  AmigaColor color;
  int frames;
};
using FlashSteps = std::vector<FlashStep>;

class PaletteFlasher {
public:
  static constexpr std::size_t MAX_STEPS = 16;

  void start(std::size_t color, FlashSteps steps);

  bool tick(AmigaPalette &palette);

  bool isFlashing() const;

private:
  std::size_t m_color = 0;
  FlashSteps m_steps;
  std::size_t m_next = 0;
  int m_countdown = 0;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_PALETTEFLASHER_H_
