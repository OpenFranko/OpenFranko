#ifndef SYSTEMS_AUDIOSYSTEM_H_
#define SYSTEMS_AUDIOSYSTEM_H_

#include <memory>
#include <string>

namespace openfranko {
namespace src {
namespace systems {

class AudioSystem {
public:
  static constexpr int ALL_VOICES = 0xF;

  AudioSystem();
  ~AudioSystem();

  AudioSystem(const AudioSystem &) = delete;
  AudioSystem &operator=(const AudioSystem &) = delete;

  void loadMusic(const std::string &path);
  void clearMusic();
  const std::string &loadedMusic() const;
  void loadSFX(const std::string &name, const std::string &path);
  void clearSFX(const std::string &name);
  void playMusic();
  void stopMusic();
  void setMusicVolume(int volume);
  void setMusicTempoScale(double scale);
  void setVblRate(int hertz);
  void setLowPassFilter(bool on);
  void playSample(const std::string &name, int voiceMask);
  void playSampleAt(const std::string &name, int voiceMask, int frequency);
  void setSampleLooping(bool looping);
  void stopSFX();
  void update();

private:
  struct Output;

  void applyTempo();

  std::unique_ptr<Output> output;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIOSYSTEM_H_
