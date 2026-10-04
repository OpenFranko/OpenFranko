#ifndef SYSTEMS_AUDIO_AUDIODEVICE_H_
#define SYSTEMS_AUDIO_AUDIODEVICE_H_

#include <cstdint>
#include <functional>
#include <memory>

namespace openfranko {
namespace src {
namespace systems {
namespace audio {

class AudioDevice {
public:
  using Render = std::function<void(int16_t *stereo, int frames)>;

  AudioDevice(int rate, Render render);
  ~AudioDevice();

  AudioDevice(const AudioDevice &) = delete;
  AudioDevice &operator=(const AudioDevice &) = delete;

  void lock();
  void unlock();
  void update();

  static bool interpolatesMusic();

private:
  struct Stream;

  std::unique_ptr<Stream> m_stream;
};

} // namespace audio
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIO_AUDIODEVICE_H_
