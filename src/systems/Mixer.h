#ifndef SYSTEMS_MIXER_H_
#define SYSTEMS_MIXER_H_

#include "Wave.h"

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {

class Mixer {
public:
  static constexpr int VOICES = 4;
  static constexpr int ALL_VOICES = 0xF;
  static constexpr int MAX_VOLUME = 64;
  static constexpr int SAMPLE_VOLUME = 56;

  explicit Mixer(int rate);
  ~Mixer();

  Mixer(const Mixer &) = delete;
  Mixer &operator=(const Mixer &) = delete;

  bool loadModule(const std::vector<char> &data);
  void releaseModule();
  void startModule();
  void stopModule();
  bool isModulePlaying() const;
  void setModuleTempo(double factor);
  void overrideModuleTempo(int tempo);
  bool isModuleTempoOverridden() const;
  void setMusicVolume(int volume);
  void setFilter(bool on);

  void play(const Sound &sound, int voiceMask, int frequency, bool loop);
  void endLoops();
  void stop(const Sound &sound);
  void stopAll();
  void update();
  bool isPlaying(int voice) const;
  bool isMusicSilenced() const;

  void render(int16_t *stereo, int frames);

private:
  struct Voice {
    const Sound *sound = nullptr;
    int frequency = 0;
    uint64_t position = 0;
    uint64_t step = 0;
    bool loop = false;
  };

  struct Playing {
    const Sound *sound;
    int frequency;
  };

  struct Module;

  using RowPosition = std::pair<int, int>;

  struct Biquad {
    double b0 = 0.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
  };

  void stopPlayer();
  void applyModuleTempo();
  RowPosition modulePosition() const;
  bool playModule(std::size_t samples);
  void followModuleTempo();
  bool isSounding(const Playing &playing) const;
  int nextSample(Voice &voice);
  void filter(int16_t *stereo, int frames);

  mutable std::mutex mutex;
  int rate;
  std::unique_ptr<Module> module;
  bool moduleLoaded = false;
  bool modulePlaying = false;
  double moduleTempoFactor = 1.0;
  int tempoOverride = 0;
  RowPosition overridePosition{-1, -1};
  std::set<RowPosition> tempoRows;
  int musicVolume;
  std::vector<int16_t> musicBuffer;
  std::array<Voice, VOICES> voices;
  std::optional<Playing> silencing;
  bool filterOn = false;
  Biquad lowPass;
  std::array<std::array<double, 4>, 2> filterHistory{};
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_MIXER_H_
