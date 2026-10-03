#include "../../../../../src/engine/street/core/SurfacePair.h"

#include <catch2/catch_all.hpp>

using namespace openfranko::src::engine::street::core;

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
