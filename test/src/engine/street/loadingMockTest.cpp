#include "../../../../src/engine/street/LoadingMock.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::src::engine::street;

namespace {

constexpr uint8_t STRIP_COLOR = 7;
constexpr uint8_t WAIT_WORD_COLOR = 5;

Picture loadingStrip() {
  Picture picture{304, 48, 0, 0, {}};
  for (int y = 0; y < 48; ++y) {
    for (int x = 0; x < 304; ++x) {
      picture.pixels.push_back(y >= 32 ? WAIT_WORD_COLOR : STRIP_COLOR);
    }
  }
  return picture;
}

Picture artwork() {
  return Picture{304, 40, 0, 0, std::vector<uint8_t>(304 * 40, 1)};
}

} // namespace

SCENARIO("Each mocked file is read under LADUJ, then unpacked under CZEKAJ") {
  GIVEN("Two files queued") {
    StatusPanel panel(loadingStrip(), artwork());
    LoadingMock loading;
    std::vector<int> loaded;
    loading.queue([&] { loaded.push_back(1); });
    loading.queue([&] { loaded.push_back(2); });

    WHEN("The first frame of loading runs") {
      const bool done = loading.advance(&panel);

      THEN("The first file is taken and the plain strip shows") {
        REQUIRE_FALSE(done);
        REQUIRE(loaded == std::vector<int>{1});
        REQUIRE(panel.surface().pixel(101, 10) == STRIP_COLOR);
      }

      AND_WHEN("Its read time has passed") {
        for (int frame = 1; frame < LoadingMock::READ_FRAMES; ++frame) {
          REQUIRE_FALSE(loading.advance(&panel));
        }
        REQUIRE(panel.surface().pixel(101, 10) == STRIP_COLOR);
        const bool unpacking = loading.advance(&panel);

        THEN("The wait word goes up for the unpack") {
          REQUIRE_FALSE(unpacking);
          REQUIRE(panel.surface().pixel(101, 10) == WAIT_WORD_COLOR);
          REQUIRE(loaded == std::vector<int>{1});
        }
      }

      AND_WHEN("The whole first file has taken its frames") {
        for (int frame = 1; frame < LoadingMock::FILE_FRAMES; ++frame) {
          REQUIRE_FALSE(loading.advance(&panel));
        }
        const bool next = loading.advance(&panel);

        THEN("The second file starts on the same frame, back on the strip") {
          REQUIRE_FALSE(next);
          REQUIRE(loaded == std::vector<int>{1, 2});
          REQUIRE(panel.surface().pixel(101, 10) == STRIP_COLOR);
        }

        AND_WHEN("The second file has taken its frames too") {
          for (int frame = 1; frame < LoadingMock::FILE_FRAMES; ++frame) {
            REQUIRE_FALSE(loading.advance(&panel));
          }

          THEN("Loading ends after exactly two files' worth of frames") {
            REQUIRE(loading.advance(&panel));
          }
        }
      }
    }
  }

  GIVEN("A file loaded while no stage is running") {
    LoadingMock loading;
    int loaded = 0;
    loading.queue([&] { ++loaded; });

    THEN("It takes the same time with no panel to draw on") {
      REQUIRE_FALSE(loading.advance(nullptr));
      REQUIRE(loaded == 1);
      for (int frame = 1; frame < LoadingMock::FILE_FRAMES; ++frame) {
        REQUIRE_FALSE(loading.advance(nullptr));
      }
      REQUIRE(loading.advance(nullptr));
    }
  }

  GIVEN("Nothing queued") {
    StatusPanel panel(loadingStrip(), artwork());
    LoadingMock loading;

    THEN("Loading is over at once and the panel is left alone") {
      REQUIRE(loading.advance(&panel));
      REQUIRE(panel.surface().pixel(101, 10) == 0);
    }
  }
}
