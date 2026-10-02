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

  GIVEN("Draws up to a limit") {
    THEN("They match std::uniform_int_distribution, including rejections") {
      const uint32_t limits[] = {0,          1,          2,          6,
                                 99,         1000,       65535,      65536,
                                 99999,      0x7FFFFFFF, 0x80000000, 0xC0000000,
                                 0xFFFFFFFE, UINT32_MAX};
      for (const uint32_t seed : SEEDS) {
        for (const uint32_t limit : limits) {
          CAPTURE(seed, limit);
          MersenneTwister twister(seed);
          MersenneTwister reference(seed);
          for (int draw = 0; draw < DRAWS; ++draw) {
            REQUIRE(
                twister.upTo(limit) ==
                std::uniform_int_distribution<uint32_t>(0, limit)(reference));
          }
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
