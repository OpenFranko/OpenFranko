#include "../../../../src/systems/graphics/ScreenMode.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <optional>
#include <vector>

using namespace openfranko::src::systems::graphics;

SCENARIO("fullscreenMode picks a mode that refreshes at the game's rate") {
  GIVEN("A 60 Hz desktop on a screen that also offers 50 Hz") {
    const std::vector<ScreenMode> modes = {
        {1920, 1080, 60}, {1920, 1080, 50}, {1280, 720, 50}, {1280, 720, 60}};
    const ScreenMode desktop{1920, 1080, 60};

    THEN("A 50 Hz game switches to the desktop size at 50 Hz") {
      REQUIRE(fullscreenMode(modes, desktop, 50) ==
              std::optional<std::size_t>(1));
    }

    THEN("A 60 Hz game keeps the desktop mode") {
      REQUIRE_FALSE(fullscreenMode(modes, desktop, 60));
    }
  }

  GIVEN("A screen that offers 50 Hz only at smaller sizes") {
    const std::vector<ScreenMode> modes = {
        {1920, 1080, 60}, {720, 576, 50}, {1280, 720, 50}};
    const ScreenMode desktop{1920, 1080, 60};

    THEN("A 50 Hz game switches to the largest of them") {
      REQUIRE(fullscreenMode(modes, desktop, 50) ==
              std::optional<std::size_t>(2));
    }
  }

  GIVEN("A laptop panel with only 60 and 48 Hz") {
    const std::vector<ScreenMode> modes = {{1920, 1080, 60}, {1920, 1080, 48}};
    const ScreenMode desktop{1920, 1080, 60};

    THEN("A 50 Hz game keeps the desktop mode") {
      REQUIRE_FALSE(fullscreenMode(modes, desktop, 50));
    }
  }

  GIVEN("A TV that reports its PAL mode as 49 Hz") {
    const std::vector<ScreenMode> modes = {{1920, 1080, 60}, {1920, 1080, 49}};
    const ScreenMode desktop{1920, 1080, 60};

    THEN("A 50 Hz game still takes it") {
      REQUIRE(fullscreenMode(modes, desktop, 50) ==
              std::optional<std::size_t>(1));
      REQUIRE(isRefreshedAt(modes[1], 50));
      REQUIRE_FALSE(isRefreshedAt(modes[1], 60));
    }
  }
}
