#ifndef TEST_JAGUAR_SAMPLER_H_
#define TEST_JAGUAR_SAMPLER_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sampler {

struct Hotspot {
  uint32_t address = 0;
  uint32_t samples = 0;
};

void start(uint32_t codeStart, uint32_t codeEnd);
void stop();
void beginFrame();
void endFrame(bool keep);
uint32_t total();
uint32_t outside();
std::vector<Hotspot> hottest(std::size_t count);

} // namespace sampler

#endif // TEST_JAGUAR_SAMPLER_H_
