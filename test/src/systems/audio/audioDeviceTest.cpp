#include "../../../../src/systems/audio/AudioDevice.h"

#include "../HeadlessSdl.h"

#include <SDL2/SDL.h>
#include <catch2/catch_all.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

using namespace openfranko::src::systems::audio;
using namespace openfranko::test::src::systems;

namespace {

constexpr int RATE = 22050;

struct Pulls {
  std::atomic<int> count{0};
  std::atomic<int> frames{0};
};

AudioDevice::Render counting(const std::shared_ptr<Pulls> &pulls) {
  return [pulls](int16_t *stereo, int frames) {
    std::fill(stereo, stereo + 2 * frames, int16_t{0});
    pulls->frames = frames;
    ++pulls->count;
  };
}

bool pulledAtLeast(const Pulls &pulls, int count) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (pulls.count < count) {
    if (std::chrono::steady_clock::now() > deadline) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return true;
}

} // namespace

SCENARIO("The device pulls stereo frames from its render callback") {
  GIVEN("A device at the game's rate") {
    HeadlessSdl sdl;
    const auto pulls = std::make_shared<Pulls>();
    AudioDevice device(RATE, counting(pulls));

    THEN("The callback is asked for whole frames") {
      REQUIRE(pulledAtLeast(*pulls, 1));
      REQUIRE(pulls->frames > 0);
    }

    WHEN("The device is locked and unlocked like a mutex") {
      {
        const std::lock_guard<AudioDevice> lock(device);
        device.update();
      }
      const int pulled = pulls->count;

      THEN("It keeps pulling afterwards") {
        REQUIRE(pulledAtLeast(*pulls, pulled + 1));
      }
    }
  }
}

SCENARIO("Closing the device lets go of its render callback") {
  GIVEN("A device that has pulled from a callback") {
    HeadlessSdl sdl;
    const auto pulls = std::make_shared<Pulls>();
    auto device = std::make_unique<AudioDevice>(RATE, counting(pulls));
    REQUIRE(pulledAtLeast(*pulls, 1));

    WHEN("It is destroyed") {
      device.reset();

      THEN("The callback is released") { REQUIRE(pulls.use_count() == 1); }
    }
  }
}

SCENARIO("A device without a sound driver reports why") {
  GIVEN("SDL told to use an audio driver that does not exist") {
    HeadlessSdl sdl;
    SDL_SetHintWithPriority(SDL_HINT_AUDIODRIVER, "openFrankoNoDriver",
                            SDL_HINT_OVERRIDE);

    THEN("Construction fails on SDL") {
      REQUIRE_THROWS_WITH(
          AudioDevice(RATE, counting(std::make_shared<Pulls>())),
          Catch::Matchers::StartsWith("Audio device error: SDL"));
    }
  }
}

SCENARIO("The SDL device wants its music interpolated") {
  GIVEN("The SDL audio device") {
    THEN("Music is interpolated") { REQUIRE(AudioDevice::interpolatesMusic()); }
  }
}
