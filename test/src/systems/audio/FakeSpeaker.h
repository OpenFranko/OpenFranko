#ifndef TEST_SRC_SYSTEMS_AUDIO_FAKESPEAKER_H_
#define TEST_SRC_SYSTEMS_AUDIO_FAKESPEAKER_H_

#include "../../../../src/systems/audio/Speaker.h"

#include <map>
#include <string>
#include <vector>

namespace openfranko {
namespace test {
namespace src {
namespace systems {
namespace audio {

class FakeSpeaker : public openfranko::src::systems::audio::Speaker {
public:
  struct Play {
    std::string name;
    int voices = 0;

    bool operator==(const Play &other) const {
      return name == other.name && voices == other.voices;
    }
  };

  std::string music;
  std::map<std::string, std::string> samples;
  std::vector<Play> plays;
  int musicStarts = 0;
  int musicOnceStarts = 0;
  int musicStops = 0;
  std::vector<int> volumes;
  std::vector<int> tempos;
  std::vector<double> tempoScales;
  std::vector<bool> filters;
  bool sampleLooping = false;

  void loadMusic(const std::string &path) override { music = path; }

  const std::string &loadedMusic() const override { return music; }

  void loadSample(const std::string &name, const std::string &path) override {
    samples[name] = path;
  }

  void clearSample(const std::string &name) override { samples.erase(name); }

  void playMusic() override { ++musicStarts; }

  void playMusicOnce() override { ++musicOnceStarts; }

  void stopMusic() override { ++musicStops; }

  void setMusicVolume(int volume) override { volumes.push_back(volume); }

  void setMusicTempoScale(double scale) override {
    tempoScales.push_back(scale);
  }

  void setMusicTempo(int tempo) override { tempos.push_back(tempo); }

  void setLowPassFilter(bool on) override { filters.push_back(on); }

  void playSample(const std::string &name, int voiceMask) override {
    plays.push_back({name, voiceMask});
  }

  void playSampleAt(const std::string &name, int voiceMask, int) override {
    plays.push_back({name, voiceMask});
  }

  void setSampleLooping(bool looping) override { sampleLooping = looping; }
};

} // namespace audio
} // namespace systems
} // namespace src
} // namespace test
} // namespace openfranko

#endif // TEST_SRC_SYSTEMS_AUDIO_FAKESPEAKER_H_
