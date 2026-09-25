#ifndef SYSTEMS_AUDIOSYSTEM_H_
#define SYSTEMS_AUDIOSYSTEM_H_

#include "AudioDevice.h"
#include "Mixer.h"
#include "Wave.h"

#include <map>
#include <memory>
#include <string>

namespace openfranko {
namespace src {
namespace systems {

class AudioSystem {
public:
  static constexpr int ALL_VOICES = Mixer::ALL_VOICES;

  AudioSystem();

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
  void applyTempo();

  Mixer mixer;
  std::map<std::string, std::unique_ptr<Sound>> sounds;
  std::string musicPath;
  double tempoScale = 1.0;
  int vblRate;
  bool sampleLooping = false;
  std::unique_ptr<AudioDevice> device;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIOSYSTEM_H_
