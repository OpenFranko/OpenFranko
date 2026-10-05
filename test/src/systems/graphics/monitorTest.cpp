#include "../../../../src/systems/graphics/Monitor.h"

#include <catch2/catch_all.hpp>

using namespace openfranko::src::systems::graphics;

namespace {

class PlainMonitor : public Monitor {
public:
  void show(const Display &display) override { shown = display.width; }
  void setNtsc(bool enabled) override { ntsc = enabled; }
  bool isNtsc() const override { return ntsc; }

  int shown = 0;
  bool ntsc = false;
};

} // namespace

SCENARIO("A monitor that only shows frames is given whole frames") {
  GIVEN("A monitor that overrides nothing but what it must") {
    PlainMonitor plain;
    const Monitor &monitor = plain;

    THEN("It reads no buffer live, shows no sprites and diffs no frames") {
      REQUIRE_FALSE(monitor.readsBuffersLive());
      REQUIRE_FALSE(monitor.showsSprites());
      REQUIRE_FALSE(monitor.diffsFrames());
    }
  }
}
