#ifndef SYSTEMS_AUDIO_MIXER_H_
#define SYSTEMS_AUDIO_MIXER_H_

#include "audio/Wave.h"

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
namespace audio {

class Mixer {
public:
  static constexpr int VOICES = 4;
  static constexpr int ALL_VOICES = 0xF;
  static constexpr int STEREO = 2;
  static constexpr int MAX_VOLUME = 64;
  static constexpr int FULL_VOLUME = 63;
  static constexpr int SAMPLE_VOLUME = 56;

  explicit Mixer(int outputRate);
  ~Mixer();

  Mixer(const Mixer &) = delete;
  Mixer &operator=(const Mixer &) = delete;

  bool loadModule(const std::vector<char> &data);
  void releaseModule();
  void startModule(bool looping = true);
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
  static constexpr int DEFAULT_MUSIC_VOLUME = 56;

  struct Voice {
    const Sound *sound = nullptr;
    int frequency = 0;
    uint64_t position = 0;
    uint64_t step = 0;
    bool loop = false;
  };

  struct Playing {
    const Sound *sound = nullptr;
    int frequency = 0;
  };

  struct Module;

  using RowPosition = std::pair<int, int>;
  using ModuleTiming = std::pair<int, int>;

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
  ModuleTiming moduleTiming() const;
  bool playModule(std::size_t samples);
  bool hasModuleEnded() const;
  void followModuleTempo();
  bool isSounding(const Playing &playing) const;
  int nextSample(Voice &voice);
  void filter(int16_t *stereo, int frames);

  mutable std::mutex m_mutex;
  int m_rate;
  std::unique_ptr<Module> m_module;
  bool m_moduleLoaded = false;
  bool m_modulePlaying = false;
  int m_moduleLoops = 0;
  double m_moduleTempoFactor = 1.0;
  int m_tempoOverride = 0;
  RowPosition m_overridePosition{-1, -1};
  RowPosition m_lastPosition{-1, -1};
  ModuleTiming m_overrideTiming{0, 0};
  std::set<RowPosition> m_tempoRows;
  int m_musicVolume = DEFAULT_MUSIC_VOLUME;
  std::vector<int16_t> m_musicBuffer;
  std::array<Voice, VOICES> m_voices;
  std::optional<Playing> m_silencing;
  bool m_filterOn = false;
  Biquad m_lowPass;
  std::array<std::array<double, 4>, 2> m_filterHistory{};
};

} // namespace audio
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIO_MIXER_H_
