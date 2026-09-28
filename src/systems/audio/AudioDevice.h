#ifndef SYSTEMS_AUDIODEVICE_H_
#define SYSTEMS_AUDIODEVICE_H_

#include <cstdint>
#include <functional>
#include <memory>

namespace openfranko {
namespace src {
namespace systems {

class AudioDevice {
public:
  using Render = std::function<void(int16_t *stereo, int frames)>;

  AudioDevice(int rate, int frames, Render render);
  ~AudioDevice();

  AudioDevice(const AudioDevice &) = delete;
  AudioDevice &operator=(const AudioDevice &) = delete;

private:
  struct Stream;

  std::unique_ptr<Stream> stream;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIODEVICE_H_
