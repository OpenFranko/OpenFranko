#ifndef ENGINE_STREET_SCENES_STREETHOST_H_
#define ENGINE_STREET_SCENES_STREETHOST_H_

#include "../../effects/color/AmigaPalette.h"
#include "../core/EndingCredits.h"
#include "../core/IndexedSurface.h"
#include "../core/LevelScript.h"
#include "../session/GameSession.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class StreetHost {
public:
  static constexpr int LOADING_STRIP = 0;
  static constexpr int PANEL_ARTWORK = 1;

  virtual ~StreetHost() = default;

  virtual std::vector<core::Picture> loadSpriteSet(int resource,
                                                   int sampleBank) = 0;
  virtual core::Picture loadPicture(int resource) = 0;
  virtual effects::color::AmigaPalette loadPalette(int resource) = 0;
  virtual std::vector<core::Picture> loadScenery(int resource) = 0;
  virtual core::LevelScript loadLevelScript(int resource) = 0;
  virtual core::EndingCredits loadEndingCredits() = 0;
  virtual core::Picture loadPanelPicture(int part) = 0;
  virtual void loadMusic(int resource) = 0;
  virtual bool isMusicLoaded(int resource) const = 0;
  virtual void playMusic() = 0;
  virtual void stopMusic() = 0;
  virtual void setMusicVolume(int volume) = 0;
  virtual void setMusicTempo(int tempo) = 0;
  virtual void playSample(int bank, int sample, int voices) = 0;
  virtual void playSampleAt(int bank, int sample, int voices,
                            int frequency) = 0;
  virtual void setSampleLoop(bool loop) = 0;
  virtual int random(int limit) = 0;
};

struct StreetInput {
  int16_t joystick = 0;
  session::SystemKey key = session::SystemKey::None;
  bool mouseButton = false;
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SCENES_STREETHOST_H_
