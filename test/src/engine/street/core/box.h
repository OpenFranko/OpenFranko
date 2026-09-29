#ifndef TEST_SRC_ENGINE_STREET_CORE_BOX_H_
#define TEST_SRC_ENGINE_STREET_CORE_BOX_H_

#include "../../../../../src/engine/street/core/IndexedSurface.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace test {
namespace src {
namespace engine {
namespace street {
namespace core {

inline openfranko::src::engine::street::core::Picture
box(int width, int height, int hotX, int hotY, uint8_t color) {
  return openfranko::src::engine::street::core::Picture{
      width, height, hotX, hotY,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height), color)};
}

inline openfranko::src::engine::street::core::Picture box(int width, int height,
                                                          uint8_t color) {
  return box(width, height, 0, 0, color);
}

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace test
} // namespace openfranko

#endif // TEST_SRC_ENGINE_STREET_CORE_BOX_H_
