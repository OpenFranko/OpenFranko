#ifndef ENGINE_STATES_SHARED_MUSICFADEOUT_H_
#define ENGINE_STATES_SHARED_MUSICFADEOUT_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/audio/Mixer.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

class MusicFadeOut {
public:
  bool advance(systems::audio::AudioSystem &audioSystem);

private:
  int m_volume = systems::audio::Mixer::FULL_VOLUME;
};

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SHARED_MUSICFADEOUT_H_
