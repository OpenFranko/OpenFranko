#include "../../../../../src/engine/street/core/IndexedSurface.h"

#include <catch2/catch_all.hpp>

#include <stdexcept>

using namespace openfranko::src::engine::street::core;

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

uint8_t expectedPixel(const Picture &picture, const IndexedSurface &before,
                      int left, int top, bool flipX, bool flipY, bool opaque,
                      int x, int y) {
  const int column = x - left;
  const int row = y - top;
  if (column < 0 || column >= picture.width || row < 0 ||
      row >= picture.height) {
    return before.pixel(x, y);
  }
  const uint8_t value = picture.at(flipX ? picture.width - 1 - column : column,
                                   flipY ? picture.height - 1 - row : row);
  return value != 0 || opaque ? value : before.pixel(x, y);
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

SCENARIO("A screen can take over pixels instead of copying them") {
  GIVEN("Pixels for a 4x3 screen") {
    std::vector<uint8_t> pixels(12);
    for (std::size_t at = 0; at < pixels.size(); ++at) {
      pixels[at] = static_cast<uint8_t>(at + 1);
    }
    const uint8_t *storage = pixels.data();

    WHEN("A screen is made from them") {
      const IndexedSurface screen(4, 3, std::move(pixels));

      THEN("It keeps the same storage and shows them row by row") {
        REQUIRE(screen.pixels().data() == storage);
        REQUIRE(screen.pixel(0, 0) == 1);
        REQUIRE(screen.pixel(3, 0) == 4);
        REQUIRE(screen.pixel(0, 2) == 9);
        REQUIRE(screen.pixel(3, 2) == 12);
      }
    }

    WHEN("Too few are given") {
      pixels.resize(5);
      const IndexedSurface screen(4, 3, std::move(pixels));

      THEN("The rest of the screen is colour 0") {
        REQUIRE(screen.pixels().size() == 12);
        REQUIRE(screen.pixel(0, 1) == 5);
        REQUIRE(screen.pixel(1, 1) == 0);
        REQUIRE(screen.pixel(3, 2) == 0);
      }
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

SCENARIO("Draw handles wide pictures four pixels at a time") {
  GIVEN("A 13 x 3 picture with opaque, transparent and mixed stretches") {
    Picture picture{13, 3, 0, 0, {}};
    for (int index = 0; index < 13 * 3; ++index) {
      picture.pixels.push_back(
          static_cast<uint8_t>(index % 9 < 4 ? 0 : index % 7 + 1));
    }
    IndexedSurface before = columns(20, 5);

    THEN("Every placement, flip and mask matches drawing pixel by pixel") {
      for (int left = -6; left <= 12; ++left) {
        for (int flags = 0; flags < 8; ++flags) {
          const bool flipX = (flags & 1) != 0;
          const bool flipY = (flags & 2) != 0;
          const bool opaque = (flags & 4) != 0;
          IndexedSurface screen = before;
          screen.draw(picture, left, 1, flipX, flipY, opaque);
          for (int y = 0; y < screen.height(); ++y) {
            for (int x = 0; x < screen.width(); ++x) {
              REQUIRE(screen.pixel(x, y) == expectedPixel(picture, before, left,
                                                          1, flipX, flipY,
                                                          opaque, x, y));
            }
          }
        }
      }
    }
  }
}

SCENARIO("A surface gets a new revision whenever its pixels may change") {
  GIVEN("A surface") {
    IndexedSurface surface(16, 8);
    const IndexedSurface other(16, 8);

    THEN("Reading it keeps the revision") {
      const uint32_t revision = surface.revision();
      REQUIRE(surface.pixel(1, 1) == 0);
      REQUIRE(surface.pixels().size() == 16 * 8);
      REQUIRE(surface.revision() == revision);
    }

    THEN("Every kind of drawing changes it") {
      uint32_t revision = surface.revision();
      const auto changed = [&] {
        const bool fresh = surface.revision() != revision;
        revision = surface.revision();
        return fresh;
      };
      surface.fill(3);
      REQUIRE(changed());
      surface.clear(1, 0, 0, 4, 4);
      REQUIRE(changed());
      surface.copy(other, 0, 0, 8, 8, 2, 2);
      REQUIRE(changed());
      surface.unpack(solid(8, 2, 5), 0, 0);
      REQUIRE(changed());
      surface.draw(solid(4, 4, 6), 3, 3, false, false);
      REQUIRE(changed());
      surface.reshape(8, 8);
      REQUIRE(changed());
      surface = other;
      REQUIRE(changed());
    }

    THEN("A copy of it has its own revision") {
      const IndexedSurface copy = surface;
      REQUIRE(copy.revision() != surface.revision());
    }
  }
}
