#ifndef SYSTEMS_AUDIO_WAVE_H_
#define SYSTEMS_AUDIO_WAVE_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {
namespace audio {

struct Sound {
  int rate = 0;
  std::vector<int8_t> frames;
};

using SignFlip = bool (*)(const uint8_t *source, int8_t *target,
                          std::size_t count);

Sound readWave(const std::vector<uint8_t> &file, SignFlip flip = nullptr);

} // namespace audio
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIO_WAVE_H_
