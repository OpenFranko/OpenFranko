#ifndef TEST_SRC_ENGINE_STREET_SCENES_FAKESTREETHOST_H_
#define TEST_SRC_ENGINE_STREET_SCENES_FAKESTREETHOST_H_

#include "../../../../../src/engine/street/scenes/StreetHost.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace openfranko {
namespace test {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class FakeStreetHost
    : public openfranko::src::engine::street::scenes::StreetHost {
public:
  using Picture = openfranko::src::engine::street::core::Picture;

  struct Sample {
    int bank;
    int sample;
    int voices;
    bool operator==(const Sample &other) const {
      return bank == other.bank && sample == other.sample &&
             voices == other.voices;
    }
  };

  std::vector<std::pair<int, int>> spriteSets;
  std::vector<int> pictures;
  std::vector<int> music;
  std::vector<int> volumes;
  int musicStarts = 0;
  int musicStops = 0;
  std::vector<Sample> samples;

  std::vector<Picture> loadSpriteSet(int resource, int sampleBank) override {
    spriteSets.emplace_back(resource, sampleBank);
    return {};
  }

  Picture loadPicture(int resource) override {
    pictures.push_back(resource);
    return {};
  }

  openfranko::src::engine::effects::color::AmigaPalette
  loadPalette(int) override {
    return {};
  }

  std::vector<Picture> loadScenery(int) override { return {}; }

  openfranko::src::engine::street::core::LevelScript
  loadLevelScript(int) override {
    return {};
  }

  openfranko::src::engine::street::core::EndingCredits
  loadEndingCredits() override {
    return {};
  }

  Picture loadPanelPicture(int) override { return {}; }

  void loadMusic(int resource) override { music.push_back(resource); }

  bool isMusicLoaded(int) const override { return false; }

  void playMusic() override { ++musicStarts; }

  void stopMusic() override { ++musicStops; }

  void setMusicVolume(int volume) override { volumes.push_back(volume); }

  void setMusicTempo(int) override {}

  void playSample(int bank, int sample, int voices) override {
    samples.push_back({bank, sample, voices});
  }

  void playSampleAt(int, int, int, int) override {}

  void setSampleLooping(bool) override {}

  int random(int) override { return 0; }

  bool played(int bank, int sample, int voices) const {
    return std::find(samples.begin(), samples.end(),
                     Sample{bank, sample, voices}) != samples.end();
  }
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace test
} // namespace openfranko

#endif // TEST_SRC_ENGINE_STREET_SCENES_FAKESTREETHOST_H_
