#ifndef ENGINE_STATES_MENU_MENUPREFETCH_H_
#define ENGINE_STATES_MENU_MENUPREFETCH_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../GameVersion.h"
#include "../../assets/PrefetchingFiles.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace menu {

class MenuPrefetch {
public:
  MenuPrefetch(assets::PrefetchingFiles &files,
               systems::audio::AudioSystem &audio);

  void start(GameVersion version);
  void pause();
  void stop();
  void step();

private:
  bool advance();

  assets::PrefetchingFiles &m_files;
  systems::audio::AudioSystem &m_audio;
  GameVersion m_version = GameVersion::V10;
  bool m_running = false;
  int m_queued = 0;
  std::string m_tunePath;
  std::unique_ptr<assets::Files::FileLoad> m_tuneRead;
  std::vector<uint8_t> m_tune;
};

} // namespace menu
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_MENU_MENUPREFETCH_H_
