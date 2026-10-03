#ifndef TEST_SRC_SYSTEMS_GRAPHICS_FAKEMONITOR_H_
#define TEST_SRC_SYSTEMS_GRAPHICS_FAKEMONITOR_H_

#include "../../../../src/systems/graphics/Display.h"
#include "../../../../src/systems/graphics/Monitor.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace test {
namespace src {
namespace systems {
namespace graphics {

class FakeMonitor : public openfranko::src::systems::graphics::Monitor {
public:
  int shows = 0;
  int width = 0;
  int height = 0;
  int displayHeight = 0;
  bool ntsc = false;
  bool live = false;
  bool sprites = false;

  void
  show(const openfranko::src::systems::graphics::Display &display) override {
    ++shows;
    width = display.width;
    height = display.height;
    displayHeight = display.displayHeight;
    m_shown = display;
    m_frame.clear();
  }

  void setNtsc(bool enabled) override { ntsc = enabled; }

  bool isNtsc() const override { return ntsc; }

  bool readsBuffersLive() const override { return live; }

  bool showsSprites() const override { return sprites; }

  uint32_t pixel(int x, int y) const {
    return frame()[static_cast<std::size_t>(y * width + x)];
  }
  const std::vector<uint32_t> &frame() const {
    if (m_frame.empty()) {
      openfranko::src::systems::graphics::rasterize(m_shown, m_frame);
    }
    return m_frame;
  }
  const openfranko::src::systems::graphics::Display &shown() const {
    return m_shown;
  }

private:
  openfranko::src::systems::graphics::Display m_shown;
  mutable std::vector<uint32_t> m_frame;
};

} // namespace graphics
} // namespace systems
} // namespace src
} // namespace test
} // namespace openfranko

#endif // TEST_SRC_SYSTEMS_GRAPHICS_FAKEMONITOR_H_
