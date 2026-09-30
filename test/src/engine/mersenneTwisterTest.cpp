#include "../../../src/engine/MersenneTwister.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <random>

using namespace openfranko::src::engine;

namespace {

constexpr int DRAWS = 2000;
constexpr uint32_t SEEDS[] = {0, 5489, 12345, UINT32_MAX};

} // namespace

SCENARIO("MersenneTwister draws the numbers of std::mt19937") {
  GIVEN("Seeds from zero to the largest") {
    THEN("Every draw matches, across several state refills") {
      for (const uint32_t seed : SEEDS) {
        CAPTURE(seed);
        MersenneTwister twister(seed);
        std::mt19937 reference(seed);
        for (int draw = 0; draw < DRAWS; ++draw) {
          REQUIRE(twister() == reference());
        }
      }
    }

    THEN("Draws from a range match as well") {
      for (const uint32_t seed : SEEDS) {
        CAPTURE(seed);
        MersenneTwister twister(seed);
        std::mt19937 reference(seed);
        for (int draw = 0; draw < DRAWS; ++draw) {
          const int limit = draw % 100;
          REQUIRE(std::uniform_int_distribution<int>(0, limit)(twister) ==
                  std::uniform_int_distribution<int>(0, limit)(reference));
        }
      }
    }
  }

  GIVEN("Any generator") {
    THEN("Its numbers span all 32 bits") {
      REQUIRE(MersenneTwister::min() == 0);
      REQUIRE(MersenneTwister::max() == UINT32_MAX);
    }
  }
}
