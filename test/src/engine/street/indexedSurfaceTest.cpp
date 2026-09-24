#include "../../../../src/engine/street/IndexedSurface.h"
#include <catch2/catch_all.hpp>
#include <stdexcept>

using namespace openfranko::src::engine::street;

namespace {

uint8_t stripe(int x) { return static_cast<uint8_t>(x % 250 + 1); }

IndexedSurface columns(int width, int height) {
  IndexedSurface surface(width, height);
  Picture stripes{width, height, 0, 0, {}};
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      stripes.pixels.push_back(stripe(x));
    }
  }
  surface.unpack(stripes, 0, 0);
  return surface;
}

Picture solid(int width, int height, uint8_t color) {
  return Picture{
      width, height, 0, 0,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height), color)};
}

} // namespace

SCENARIO("Screen Copy clips as Sco0 does") {
  GIVEN("A 320 x 222 screen striped by column") {
    IndexedSurface screen = columns(320, 222);

    WHEN("Def Scroll 1 scrolls it 8 px to the left") {
      screen.copy(screen, 0, 0, 320, 222, -8, 0);

      THEN("Columns 8 to 319 land on 0 to 311 and the last 8 are left as-is") {
        REQUIRE(screen.pixel(0, 10) == stripe(8));
        REQUIRE(screen.pixel(311, 10) == stripe(319));
        REQUIRE(screen.pixel(312, 10) == stripe(312));
        REQUIRE(screen.pixel(319, 10) == stripe(319));
      }
    }

    WHEN("Def Scroll 2 scrolls it 8 px to the right") {
      screen.copy(screen, 0, 0, 320, 222, 8, 0);

      THEN("The copy runs backwards, so nothing is smeared") {
        REQUIRE(screen.pixel(8, 0) == stripe(0));
        REQUIRE(screen.pixel(319, 0) == stripe(311));
        REQUIRE(screen.pixel(0, 0) == stripe(0));
      }
    }
  }

  GIVEN("A panel and a glyph below its visible rows") {
    IndexedSurface panel = columns(304, 48);

    THEN("A block copies with exclusive ends") {
      panel.copy(panel, 8, 32, 15, 39, 48, 9);
      REQUIRE(panel.pixel(48, 9) == stripe(8));
      REQUIRE(panel.pixel(54, 15) == stripe(14));
      REQUIRE(panel.pixel(55, 9) == stripe(55));
      REQUIRE(panel.pixel(48, 16) == stripe(48));
    }

    THEN("A negative source origin shifts the destination with it") {
      panel.copy(panel, -3, 0, 10, 1, 100, 20);
      REQUIRE(panel.pixel(103, 20) == stripe(0));
      REQUIRE(panel.pixel(102, 20) == stripe(102));
    }

    THEN("An empty or inverted rectangle copies nothing") {
      const auto before = panel.pixels();
      panel.copy(panel, 80, 32, 78, 41, 6, 8);
      panel.copy(panel, 400, 0, 500, 5, 0, 0);
      REQUIRE(panel.pixels() == before);
    }
  }
}

SCENARIO("Cls fills a rectangle with exclusive ends, clamped to the screen") {
  GIVEN("A blank panel") {
    IndexedSurface panel(304, 48);
    panel.clear(6, 111 + 60, 13, 176, 15);

    THEN("It fills x 171 to 175 on rows 13 and 14 only") {
      REQUIRE(panel.pixel(170, 13) == 0);
      REQUIRE(panel.pixel(171, 13) == 6);
      REQUIRE(panel.pixel(175, 14) == 6);
      REQUIRE(panel.pixel(176, 13) == 0);
      REQUIRE(panel.pixel(171, 15) == 0);
    }

    THEN("Coordinates outside the screen are clamped") {
      panel.clear(3, -10, -10, 1000, 2);
      REQUIRE(panel.pixel(0, 0) == 3);
      REQUIRE(panel.pixel(303, 1) == 3);
      REQUIRE(panel.pixel(0, 2) == 0);
    }
  }
}

SCENARIO("Unpack draws a packed picture opaquely at a byte-aligned X") {
  GIVEN("A screen and a 16 px column of colour 0 and 5") {
    IndexedSurface screen(320, 222);
    screen.fill(9);
    Picture column = solid(16, 222, 5);
    column.pixels[0] = 0;

    WHEN("It is unpacked at x 307") {
      screen.unpack(column, 307, 0);

      THEN("X is rounded down to 304 and colour 0 is drawn too") {
        REQUIRE(screen.pixel(304, 0) == 0);
        REQUIRE(screen.pixel(305, 0) == 5);
        REQUIRE(screen.pixel(303, 0) == 9);
      }
    }

    THEN("A picture that does not fit is refused, not clipped") {
      REQUIRE_THROWS_AS(screen.unpack(column, 312, 0), std::out_of_range);
      REQUIRE_THROWS_AS(screen.unpack(column, 0, 1), std::out_of_range);
    }
  }
}

SCENARIO("Draw is masked, flipped and clipped") {
  GIVEN("A 3 x 2 picture with one transparent pixel") {
    const Picture picture{3, 2, 0, 0, {1, 2, 0, 4, 5, 6}};
    IndexedSurface screen(8, 4);
    screen.fill(9);

    THEN("Colour 0 lets the screen show through") {
      screen.draw(picture, 1, 1, false, false);
      REQUIRE(screen.pixel(1, 1) == 1);
      REQUIRE(screen.pixel(3, 1) == 9);
      REQUIRE(screen.pixel(3, 2) == 6);
    }

    THEN("Both flips mirror the pixels") {
      screen.draw(picture, 0, 0, true, true);
      REQUIRE(screen.pixel(0, 0) == 6);
      REQUIRE(screen.pixel(2, 0) == 4);
      REQUIRE(screen.pixel(0, 1) == 9);
      REQUIRE(screen.pixel(2, 1) == 1);
    }

    THEN("Pixels off the screen are dropped") {
      screen.draw(picture, -2, 3, false, false);
      REQUIRE(screen.pixel(0, 3) == 9);
      REQUIRE(screen.intersects(-2, 3, 3, 2));
      REQUIRE_FALSE(screen.intersects(-3, 3, 3, 2));
      REQUIRE_FALSE(screen.intersects(8, 0, 3, 2));
    }
  }
}
