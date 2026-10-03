#ifndef SYSTEMS_AUDIO_SPEAKER_H_
#define SYSTEMS_AUDIO_SPEAKER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace audio {

class Speaker {
public:
  class MusicLoad {
  public:
    virtual ~MusicLoad() = default;
    virtual bool step() = 0;
  };

  virtual ~Speaker() = default;

  virtual void loadMusic(const std::string &path) = 0;
  virtual std::unique_ptr<MusicLoad> beginMusic(const std::string &path,
                                                std::vector<uint8_t>, int) {
    return std::make_unique<WholeMusic>(*this, path);
  }
  virtual const std::string &loadedMusic() const = 0;
  virtual void loadSample(const std::string &name, const std::string &path) = 0;
  virtual void clearSample(const std::string &name) = 0;
  virtual void playMusic() = 0;
  virtual void playMusicOnce() = 0;
  virtual void stopMusic() = 0;
  virtual void setMusicVolume(int volume) = 0;
  virtual void setMusicTempoScale(double scale) = 0;
  virtual void setMusicTempo(int tempo) = 0;
  virtual void setLowPassFilter(bool on) = 0;
  virtual void playSample(const std::string &name, int voiceMask) = 0;
  virtual void playSampleAt(const std::string &name, int voiceMask,
                            int frequency) = 0;
  virtual void setSampleLooping(bool looping) = 0;

private:
  class WholeMusic : public MusicLoad {
  public:
    WholeMusic(Speaker &speaker, std::string path)
        : m_speaker(speaker), m_path(std::move(path)) {}

    bool step() override {
      m_speaker.loadMusic(m_path);
      return true;
    }

  private:
    Speaker &m_speaker;
    std::string m_path;
  };
};

} // namespace audio
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIO_SPEAKER_H_
