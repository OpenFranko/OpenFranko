#include "../../../../../src/engine/street/core/SurfacePair.h"

#include "box.h"

#include <catch2/catch_all.hpp>

#include <cstdint>

using namespace openfranko::src::engine::street::core;
using namespace openfranko::test::src::engine::street::core;

namespace {

constexpr int WIDTH = 32;
constexpr int HEIGHT = 16;
constexpr int BOB = 1;
constexpr int IMAGE = 1;
constexpr int BOB_SIZE = 4;
constexpr uint8_t BACKGROUND = 1;
constexpr uint8_t PAINT = 2;
constexpr uint8_t BOB_COLOR = 3;

} // namespace

SCENARIO("A surface pair never composes into the shown surface") {
  GIVEN("A pair whose first frame was composed and shown") {
    SurfacePair pair(4, 2);
    pair.compose().fill(1);
    const IndexedSurface &shown = pair.shown();

    WHEN("The next frame is composed") {
      IndexedSurface &next = pair.compose();
      next.fill(2);

      THEN("It is the other surface, and the shown one is unchanged") {
        REQUIRE(&next != &shown);
        REQUIRE(shown.pixel(0, 0) == 1);
        REQUIRE(&pair.shown() == &next);
      }
    }

    WHEN("A frame is composed twice before it is shown") {
      IndexedSurface &first = pair.compose();
      IndexedSurface &second = pair.compose();

      THEN("Both go to the same hidden surface") {
        REQUIRE(&first == &second);
        REQUIRE(&first != &shown);
      }
    }
  }
}

SCENARIO("A surface pair recomposes a screen and bobs only when they change") {
  GIVEN("A screen with a bob over it, composed and shown") {
    SurfacePair pair(WIDTH, HEIGHT);
    IndexedSurface screen(WIDTH, HEIGHT);
    screen.fill(BACKGROUND);
    ImageBank images;
    images.load(IMAGE, {box(BOB_SIZE, BOB_SIZE, BOB_COLOR)});
    BobLayer bobs;
    bobs.set(BOB, 8, 4, IMAGE);
    pair.compose(screen, bobs, images);
    const IndexedSurface &composed = pair.shown();
    const uint32_t revision = composed.revision();

    THEN("The shown surface holds the screen with the bob drawn over it") {
      REQUIRE(composed.pixel(0, 0) == BACKGROUND);
      REQUIRE(composed.pixel(8, 4) == BOB_COLOR);
      REQUIRE(composed.pixel(11, 7) == BOB_COLOR);
      REQUIRE(composed.pixel(12, 4) == BACKGROUND);
    }

    WHEN("They are composed again unchanged") {
      pair.compose(screen, bobs, images);

      THEN("Nothing is redrawn: the same surface is shown as it was") {
        REQUIRE(&pair.shown() == &composed);
        REQUIRE(pair.shown().revision() == revision);
      }
    }

    WHEN("The screen changes") {
      screen.clear(PAINT, 0, 0, 2, 2);
      pair.compose(screen, bobs, images);

      THEN("The other surface shows the change with the bob over it") {
        const IndexedSurface &next = pair.shown();
        REQUIRE(&next != &composed);
        REQUIRE(next.pixel(0, 0) == PAINT);
        REQUIRE(next.pixel(8, 4) == BOB_COLOR);
      }
    }

    WHEN("The bob moves") {
      bobs.set(BOB, 20, 4, IMAGE);
      pair.compose(screen, bobs, images);

      THEN("It is drawn at its new place only") {
        const IndexedSurface &next = pair.shown();
        REQUIRE(next.pixel(20, 4) == BOB_COLOR);
        REQUIRE(next.pixel(8, 4) == BACKGROUND);
      }
    }

    WHEN("The bob is turned off") {
      bobs.off(BOB);
      pair.compose(screen, bobs, images);

      THEN("The screen is shown without it") {
        REQUIRE(pair.shown().pixel(8, 4) == BACKGROUND);
      }
    }

    WHEN("Another screen is composed") {
      IndexedSurface other(WIDTH, HEIGHT);
      other.fill(PAINT);
      pair.compose(other, bobs, images);

      THEN("It is shown with the bob") {
        REQUIRE(pair.shown().pixel(0, 0) == PAINT);
        REQUIRE(pair.shown().pixel(8, 4) == BOB_COLOR);
      }
    }
  }
}
