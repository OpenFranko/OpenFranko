#ifndef ENGINE_STATES_ENGINESTREETHOST_H_
#define ENGINE_STATES_ENGINESTREETHOST_H_

#include "../../../systems/AudioSystem.h"
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
  explicit EngineStreetHost(systems::AudioSystem &audioSystem,
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
  void playMusic() override;
  void stopMusic() override;
  void setMusicVolume(int volume) override;
  void playSample(int bank, int sample, int voices) override;
  void playSampleAt(int bank, int sample, int voices, int frequency) override;
  void setSampleLoop(bool loop) override;
  int random(int limit) override;

  static std::string sampleName(int bank, int sample);

private:
  std::string resourcePath(int resource) const;
  std::vector<street::Picture> loadFrames(int resource) const;
  void loadSamples(int resource, int bank);
  void clearSamples(int bank);

  systems::AudioSystem &m_audioSystem;
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
