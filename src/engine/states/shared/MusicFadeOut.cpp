#include "MusicFadeOut.h"

namespace openfranko::src::engine::states::shared {

bool MusicFadeOut::advance(systems::audio::AudioSystem &audioSystem) {
  if (m_volume >= 0) {
    audioSystem.setMusicVolume(m_volume--);
    return false;
  }
  audioSystem.stopMusic();
  return true;
}

} // namespace openfranko::src::engine::states::shared
