#include "../../../../src/engine/effects/AmalMotion.h"
#include <catch2/catch_all.hpp>
#include <cstdint>
#include <vector>

using namespace openfranko::src::engine::effects;

namespace {

std::vector<int16_t> run(AmalMotion &motion, int16_t position, int frames) {
  std::vector<int16_t> positions;
  for (int frame = 0; frame < frames; ++frame) {
    position = motion.advance(position);
    positions.push_back(position);
  }
  return positions;
}

} // namespace

SCENARIO("AmalMotion moves like the AMAL M instruction") {
  GIVEN("A menu icon flying in from the left: M88,0,16;M8,0,8;") {
    AmalMotion motion({{88, 16}, {8, 8}});

    THEN("It follows the reference VM frame by frame and stops at 32") {
      REQUIRE(run(motion, -64, 26) ==
              std::vector<int16_t>{-58, -53, -47, -42, -36, -31, -25, -20, -14,
                                   -9,  -3,  2,   8,   13,  19,  24,  25,  26,
                                   27,  28,  29,  30,  31,  32,  32,  32});
      REQUIRE(motion.isFinished());
    }
  }

  GIVEN("A menu icon flying in from the right: M-88,0,16;M-8,0,8;") {
    AmalMotion motion({{-88, 16}, {-8, 8}});

    THEN("It follows the reference VM frame by frame and stops at 288") {
      REQUIRE(run(motion, 384, 24) ==
              std::vector<int16_t>{379, 373, 368, 362, 357, 351, 346, 340,
                                   335, 329, 324, 318, 313, 307, 302, 296,
                                   295, 294, 293, 292, 291, 290, 289, 288});
    }
  }

  GIVEN("A menu icon flying out to the left from 32") {
    AmalMotion motion({{-88, 16}, {-8, 8}});

    THEN("It rounds towards minus infinity as the 68000 does") {
      REQUIRE(run(motion, 32, 8) ==
              std::vector<int16_t>{27, 21, 16, 10, 5, -1, -6, -12});
      REQUIRE(run(motion, -12, 16).back() == -64);
    }
  }

  GIVEN("The pointing hand's waggle: four rounds of M4,0,2;M-4,0,2; and a "
        "Next that costs a frame") {
    AmalMotion motion({{4, 2},
                       {-4, 2},
                       {0, 1},
                       {4, 2},
                       {-4, 2},
                       {0, 1},
                       {4, 2},
                       {-4, 2},
                       {0, 1},
                       {4, 2},
                       {-4, 2},
                       {0, 1}});

    THEN("It follows the reference VM and ends where it started") {
      const auto positions = run(motion, 128, 21);
      const std::vector<int16_t> round = {130, 132, 130, 128, 128};
      for (int i = 0; i < 20; ++i) {
        REQUIRE(positions[i] == round[i % 5]);
      }
      REQUIRE(positions[20] == 128);
      REQUIRE(motion.isFinished());
    }
  }

  GIVEN("A move over zero frames") {
    AmalMotion motion({{10, 0}});

    THEN("It is clamped to one frame, as AMAL does") {
      REQUIRE(motion.advance(0) == 10);
      REQUIRE(motion.isFinished());
    }
  }

  GIVEN("No moves at all") {
    AmalMotion motion;

    THEN("It leaves the position alone") {
      REQUIRE(motion.isFinished());
      REQUIRE(motion.advance(77) == 77);
    }
  }
}
