#ifndef ENGINE_STATES_ENGINESTREETHOST_H_
#define ENGINE_STATES_ENGINESTREETHOST_H_

#include "../../../systems/AudioSystem.h"
#include "../../GameVersion.h"
#include "../../street/StreetStage.h"

#include <map>
#include <random>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace level1 {

class EngineStreetHost : public street::StreetHost {
public:
  EngineStreetHost(systems::AudioSystem &audioSystem, GameVersion version,
                   std::string directory = "assets");
  ~EngineStreetHost() override;

  std::vector<street::Picture> loadSpriteSet(int resource,
                                             int sampleBank) override;
  street::Picture loadPicture(int resource) override;
  effects::AmigaPalette loadPalette(int resource) override;
  std::vector<street::Picture> loadScenery(int resource) override;
  street::LevelScript loadLevelScript(int resource) override;
  street::EndingCredits loadEndingCredits() override;
  street::Picture loadPanelPicture(int part) override;
  void loadMusic(int resource) override;
  bool isMusicLoaded(int resource) const override;
  void playMusic() override;
  void stopMusic() override;
  void setMusicVolume(int volume) override;
  void setMusicTempo(int tempo) override;
  void playSample(int bank, int sample, int voices) override;
  void playSampleAt(int bank, int sample, int voices, int frequency) override;
  void setSampleLoop(bool loop) override;
  int random(int limit) override;

  GameVersion version() const;

  static std::string sampleName(int bank, int sample);

private:
  std::string resourceName(int resource) const;
  std::string resourcePath(int resource) const;
  std::string musicPath(int resource) const;
  std::vector<street::Picture> loadFrames(int resource) const;
  void loadSamples(int resource, int bank);
  void clearSamples(int bank);

  systems::AudioSystem &m_audioSystem;
  GameVersion m_version;
  std::string m_directory;
  std::mt19937 m_random;
  std::map<int, std::vector<int>> m_samples;
};

} // namespace level1
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_ENGINESTREETHOST_H_
