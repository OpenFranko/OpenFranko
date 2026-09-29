#include "MusicFadeOut.h"

namespace openfranko::src::engine::states::shared {

bool MusicFadeOut::advance(systems::audio::Speaker &speaker) {
  if (m_volume >= 0) {
    speaker.setMusicVolume(m_volume--);
    return false;
  }
  speaker.stopMusic();
  return true;
}

} // namespace openfranko::src::engine::states::shared
