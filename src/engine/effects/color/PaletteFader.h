#ifndef ENGINE_EFFECTS_PALETTEFADER_H_
#define ENGINE_EFFECTS_PALETTEFADER_H_

#include "AmigaPalette.h"

#include <cstddef>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

class PaletteFader {
public:
  static constexpr AmigaColor KEEP = 0xFFFF;

  void start(const AmigaPalette &palette, int speed,
             const AmigaPalette &target);

  bool tick(AmigaPalette &palette);

  bool isFading() const;

private:
  AmigaPalette m_current;
  AmigaPalette m_target;
  std::vector<std::size_t> m_fading;
  int m_speed = 0;
  int m_countdown = 0;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_PALETTEFADER_H_
