#ifndef SYSTEMS_AUDIO_WAVE_H_
#define SYSTEMS_AUDIO_WAVE_H_

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

Sound readWave(const std::vector<uint8_t> &file);

} // namespace audio
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_AUDIO_WAVE_H_
