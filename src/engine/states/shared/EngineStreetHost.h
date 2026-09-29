#ifndef ENGINE_STATES_SHARED_ENGINESTREETHOST_H_
#define ENGINE_STATES_SHARED_ENGINESTREETHOST_H_

#include "../../../systems/audio/Speaker.h"
#include "../../GameVersion.h"
#include "../../street/scenes/StreetHost.h"

#include <map>
#include <random>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

class EngineStreetHost : public street::scenes::StreetHost {
public:
  EngineStreetHost(systems::audio::Speaker &speaker, GameVersion version,
                   std::string directory = "assets");
  ~EngineStreetHost() override;

  std::vector<street::core::Picture> loadSpriteSet(int resource,
                                                   int sampleBank) override;
  street::core::Picture loadPicture(int resource) override;
  effects::color::AmigaPalette loadPalette(int resource) override;
  std::vector<street::core::Picture> loadScenery(int resource) override;
  street::core::LevelScript loadLevelScript(int resource) override;
  street::core::EndingCredits loadEndingCredits() override;
  street::core::Picture loadPanelPicture(int part) override;
  void loadMusic(int resource) override;
  bool isMusicLoaded(int resource) const override;
  void playMusic() override;
  void stopMusic() override;
  void setMusicVolume(int volume) override;
  void setMusicTempo(int tempo) override;
  void playSample(int bank, int sample, int voices) override;
  void playSampleAt(int bank, int sample, int voices, int frequency) override;
  void setSampleLooping(bool loop) override;
  int random(int limit) override;

  GameVersion version() const;

  static std::string sampleName(int bank, int sample);

private:
  std::string resourceName(int resource) const;
  std::string resourcePath(int resource) const;
  std::string musicPath(int resource) const;
  std::vector<street::core::Picture> loadFrames(int resource) const;
  void loadSamples(int resource, int bank);
  void clearSamples(int bank);

  systems::audio::Speaker &m_speaker;
  GameVersion m_version;
  std::string m_directory;
  std::mt19937 m_random;
  std::map<int, std::vector<int>> m_samples;
};

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SHARED_ENGINESTREETHOST_H_
