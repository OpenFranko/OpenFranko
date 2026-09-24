#include "../../../../src/engine/effects/AmalAnim.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::src::engine::effects;

namespace {

std::vector<int> run(AmalAnim &anim, int image, int frames) {
  std::vector<int> images;
  for (int frame = 0; frame < frames; ++frame) {
    image = anim.advance(image);
    images.push_back(image);
  }
  return images;
}

} // namespace

SCENARIO("AmalAnim animates like the AMAL A instruction") {
  GIVEN("The character select's blink: A2,(0,10)(2,10)") {
    AmalAnim anim({{0, 10}, {2, 10}}, 2);
    const auto images = run(anim, 0, 45);

    THEN("Like AmAni and AmDoAni: each image 10 frames, then the last stays") {
      for (int frame = 0; frame < 10; ++frame) {
        REQUIRE(images[frame] == 0);
      }
      for (int frame = 10; frame < 20; ++frame) {
        REQUIRE(images[frame] == 2);
      }
      for (int frame = 20; frame < 30; ++frame) {
        REQUIRE(images[frame] == 0);
      }
      for (int frame = 30; frame < 45; ++frame) {
        REQUIRE(images[frame] == 2);
      }
      REQUIRE(anim.isFinished());
    }
  }

  GIVEN("An animation with no loop count") {
    AmalAnim anim({{5, 1}, {6, 1}}, 0);

    THEN("It never stops, the first image shown on the very first frame") {
      REQUIRE(run(anim, 0, 6) == std::vector<int>{5, 6, 5, 6, 5, 6});
      REQUIRE_FALSE(anim.isFinished());
    }
  }

  GIVEN("A frame with no delay") {
    AmalAnim anim({{7, 0}, {8, 0}}, 1);

    THEN("It is clamped to one frame") {
      REQUIRE(run(anim, 0, 3) == std::vector<int>{7, 8, 8});
    }
  }

  GIVEN("No animation at all") {
    AmalAnim anim;

    THEN("The image is left alone") {
      REQUIRE(anim.isFinished());
      REQUIRE(anim.advance(42) == 42);
    }
  }
}
