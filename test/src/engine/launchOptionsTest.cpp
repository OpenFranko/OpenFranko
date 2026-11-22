#include "../../../src/engine/LaunchOptions.h"

#include <catch2/catch_all.hpp>

#include <stdexcept>

using namespace openfranko::src::engine;

SCENARIO("parseLaunchOptions opens the game fullscreen only when asked") {
  GIVEN("A command line with only the program name") {
    const char *const arguments[] = {"OpenFranko"};

    THEN("The game opens in a window") {
      REQUIRE_FALSE(parseLaunchOptions(1, arguments).fullscreen);
    }
  }

  GIVEN("No command line at all, as the Jaguar starts the game") {
    THEN("The game opens in a window") {
      REQUIRE_FALSE(parseLaunchOptions(0, nullptr).fullscreen);
    }
  }

  GIVEN("A command line with -f") {
    const char *const arguments[] = {"OpenFranko", "-f"};

    THEN("The game opens fullscreen") {
      REQUIRE(parseLaunchOptions(2, arguments).fullscreen);
    }
  }

  GIVEN("A command line with --fullscreen") {
    const char *const arguments[] = {"OpenFranko", "--fullscreen"};

    THEN("The game opens fullscreen") {
      REQUIRE(parseLaunchOptions(2, arguments).fullscreen);
    }
  }

  GIVEN("A command line with both spellings") {
    const char *const arguments[] = {"OpenFranko", "--fullscreen", "-f"};

    THEN("The game opens fullscreen") {
      REQUIRE(parseLaunchOptions(3, arguments).fullscreen);
    }
  }
}

SCENARIO("parseLaunchOptions refuses an option it does not know") {
  GIVEN("A command line with a misspelt option") {
    const char *const arguments[] = {"./OpenFranko", "--fulscreen"};

    THEN("It names the option and shows the usage") {
      REQUIRE_THROWS_AS(parseLaunchOptions(2, arguments),
                        std::invalid_argument);
      REQUIRE_THROWS_WITH(parseLaunchOptions(2, arguments),
                          "Unknown option: --fulscreen\n"
                          "Usage: ./OpenFranko [-f|--fullscreen]");
    }
  }

  GIVEN("A command line with an unknown option after -f") {
    const char *const arguments[] = {"OpenFranko", "-f", "fullscreen"};

    THEN("It names the unknown one") {
      REQUIRE_THROWS_WITH(parseLaunchOptions(3, arguments),
                          "Unknown option: fullscreen\n"
                          "Usage: OpenFranko [-f|--fullscreen]");
    }
  }
}
