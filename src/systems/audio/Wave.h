#ifndef SYSTEMS_WAVE_H_
#define SYSTEMS_WAVE_H_

#include <cstdint>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {

struct Sound {
  int rate = 0;
  std::vector<int8_t> frames;
};

Sound readWave(const std::vector<uint8_t> &file);
Sound loadWave(const std::string &path);

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_WAVE_H_
