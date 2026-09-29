#ifndef TEST_SRC_ENGINE_AMAL_RUN_H_
#define TEST_SRC_ENGINE_AMAL_RUN_H_

#include "../../../../src/engine/amal/Machine.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace openfranko {
namespace test {
namespace src {
namespace engine {
namespace amal {

struct Run {
  std::vector<int16_t> xs;
  std::vector<int16_t> ys;
  std::vector<int16_t> images;
};

inline Run run(openfranko::src::engine::amal::Machine &machine,
               const openfranko::src::engine::amal::Object &object,
               int frames) {
  Run result;
  for (int frame = 0; frame < frames; ++frame) {
    machine.tick();
    result.xs.push_back(object.x);
    result.ys.push_back(object.y);
    result.images.push_back(object.image);
  }
  return result;
}

inline std::vector<std::pair<int16_t, int>>
holds(const std::vector<int16_t> &values) {
  std::vector<std::pair<int16_t, int>> result;
  for (int16_t value : values) {
    if (result.empty() || result.back().first != value) {
      result.emplace_back(value, 0);
    }
    ++result.back().second;
  }
  return result;
}

} // namespace amal
} // namespace engine
} // namespace src
} // namespace test
} // namespace openfranko

#endif // TEST_SRC_ENGINE_AMAL_RUN_H_
