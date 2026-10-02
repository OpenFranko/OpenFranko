#ifndef SYSTEMS_AUDIO_AUDIOSYSTEM_H_
#define SYSTEMS_AUDIO_AUDIOSYSTEM_H_

#include "audio/Speaker.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace audio {

struct S3mModule;

class AudioSystem : public Speaker {
public:
  using Read = std::function<std::vector<uint8_t>(const std::string &path)>;

  explicit AudioSystem(Read read);
  ~AudioSystem() override;

  AudioSystem(const AudioSystem &) = delete;
  AudioSystem &operator=(const AudioSystem &) = delete;

  void loadMusic(const std::string &path) override;
  std::unique_ptr<MusicLoad> beginMusic(const std::string &path,
                                        std::vector<uint8_t> data,
                                        int steps) override;
  void clearMusic();
  void prepareMusic(const std::string &path, std::vector<uint8_t> data);
  bool stepPreparation();
  void dropPreparedMusic();
  const std::string &loadedMusic() const override;
  void loadSample(const std::string &name, const std::string &path) override;
  void clearSample(const std::string &name) override;
  void playMusic() override;
  void playMusicOnce() override;
  void stopMusic() override;
  void setMusicVolume(int volume) override;
  void setMusicTempoScale(double scale) override;
  void setMusicTempo(int tempo) override;
  void setVblRate(int hertz);
  void setLowPassFilter(bool on) override;
  void playSample(const std::string &name, int voiceMask) override;
  void playSampleAt(const std::string &name, int voiceMask,
                    int frequency) override;
  void setSampleLooping(bool looping) override;
  void stopSamples();
  void update();

private:
  struct Output;
  class MusicSteps;

  void installMusic(const std::string &path, std::unique_ptr<S3mModule> module);
  void startMusic(bool looping);
  void applyTempo();

  std::unique_ptr<Output> m_output;
};

} // namespace audio
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIO_AUDIOSYSTEM_H_
