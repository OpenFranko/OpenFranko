#ifndef ENGINE_STATES_PRESENTS_MUSICFADEOUT_H_
#define ENGINE_STATES_PRESENTS_MUSICFADEOUT_H_

#include "../../../systems/AudioSystem.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace presents {

class MusicFadeOut {
public:
  static constexpr int FULL_VOLUME = 63;

  bool advance(systems::AudioSystem &audioSystem);

private:
  int m_volume = FULL_VOLUME;
};

} // namespace presents
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_PRESENTS_MUSICFADEOUT_H_
