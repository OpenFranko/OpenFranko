#ifndef ENGINE_STATES_SHARED_ENGINESTREETHOST_H_
#define ENGINE_STATES_SHARED_ENGINESTREETHOST_H_

#include "../../../systems/audio/Speaker.h"
#include "../../GameVersion.h"
#include "../../MersenneTwister.h"
#include "../../assets/Files.h"
#include "../../street/scenes/StreetHost.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace shared {

class EngineStreetHost : public street::scenes::StreetHost {
public:
  EngineStreetHost(systems::audio::Speaker &speaker, assets::Files &files,
                   GameVersion version, MersenneTwister &random,
                   std::function<void()> yield,
                   std::string directory = "assets");
  ~EngineStreetHost() override;

  std::vector<street::core::Picture> loadSpriteSet(int resource,
                                                   int sampleBank) override;
  std::unique_ptr<SpriteSetLoad> beginSpriteSet(int resource, int sampleBank,
                                                int base) override;
  std::unique_ptr<FramesLoad> beginScenery(int resource) override;
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
  void yield() override;

  GameVersion version() const;

  static std::string sampleName(int bank, int sample);

private:
  class SetListing;
  class SpriteSetSteps;
  class ScenerySteps;

  using NumberedFiles = std::vector<std::pair<int, std::string>>;

  std::string resourceName(int resource) const;
  street::core::Picture loadFrame(const std::string &path) const;
  void loadSample(int bank, int sample, const std::string &path);
  std::string resourcePath(int resource) const;
  std::string musicPath(int resource) const;
  std::vector<street::core::Picture> loadFrames(int resource) const;
  void loadSamples(int resource, int bank);
  void clearSamples(int bank);
  bool clearFirstSample(int bank);
  const std::string &cachedSampleName(int bank, int sample);

  systems::audio::Speaker &m_speaker;
  assets::Files &m_files;
  GameVersion m_version;
  std::function<void()> m_yield;
  std::string m_directory;
  MersenneTwister &m_random;
  std::map<int, std::vector<int>> m_samples;
  std::vector<std::vector<std::string>> m_sampleNames;
};

} // namespace shared
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SHARED_ENGINESTREETHOST_H_
