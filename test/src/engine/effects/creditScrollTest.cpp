#include "../../../../src/engine/effects/CreditScroll.h"
#include <catch2/catch_all.hpp>
#include <cstdint>
#include <vector>

using namespace openfranko::src::engine::effects;

namespace {

struct Frame {
  int16_t y;
  int image;
};

std::vector<Frame> run(CreditScroll &scroll, int frames) {
  std::vector<Frame> out;
  for (int frame = 0; frame < frames; ++frame) {
    scroll.advance();
    out.push_back({scroll.y(), scroll.image()});
  }
  return out;
}

} // namespace

SCENARIO("CreditScroll runs the menu's credit AMAL program") {
  GIVEN("The first credit, starting at y 350 with images 55 to 67") {
    CreditScroll scroll(350, 55, 67);
    const auto frames = run(scroll, 3901);

    THEN("It rises one pixel every second frame, as the reference VM does") {
      const std::vector<int16_t> expected = {350, 349, 349, 348, 348, 347,
                                             347, 346, 346, 345, 345, 344};
      for (std::size_t i = 0; i < expected.size(); ++i) {
        REQUIRE(frames[i].y == expected[i]);
      }
    }

    THEN("Past y -40 it drops back to 350 with the image three further on") {
      REQUIRE(frames[779].y == -40);
      REQUIRE(frames[779].image == 55);
      REQUIRE(frames[780].y == 350);
      REQUIRE(frames[780].image == 58);
      REQUIRE(frames[781].y == 349);
    }

    THEN("Its images cycle 55, 58, 61, 64, 67 and back to 55") {
      REQUIRE(frames[1560].image == 61);
      REQUIRE(frames[2340].image == 64);
      REQUIRE(frames[3120].image == 67);
      REQUIRE(frames[3900].image == 55);
      REQUIRE(frames[3900].y == 350);
    }
  }

  GIVEN("The second credit, starting lower at y 490") {
    CreditScroll scroll(490, 56, 68);
    const auto frames = run(scroll, 1061);

    THEN("Its first pass is longer, then it wraps like the others") {
      REQUIRE(frames[1059].y == -40);
      REQUIRE(frames[1060].y == 350);
      REQUIRE(frames[1060].image == 59);
    }
  }
}
