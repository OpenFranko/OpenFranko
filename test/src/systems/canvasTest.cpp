#include "../../../src/systems/Canvas.h"
#include <catch2/catch_all.hpp>

#include <cstdint>
#include <utility>
#include <vector>

using namespace openfranko::src::systems;

namespace {

constexpr uint8_t FILL = Canvas::FILL_INDEX;

IndexedBitmap picture(int width, int height, std::vector<uint8_t> pixels,
                      int hotX = 0, int hotY = 0) {
  return IndexedBitmap{width, height, hotX, hotY, std::move(pixels), {}};
}

uint8_t at(const Canvas &canvas, int x, int y) {
  return canvas.pixels()[static_cast<std::size_t>(y * canvas.width() + x)];
}

std::vector<uint32_t> shown(const Canvas &canvas) {
  std::vector<uint32_t> argb;
  rasterize(canvas.output(), argb);
  return argb;
}

} // namespace

SCENARIO("A canvas is an 8-bit screen with its own palette") {
  GIVEN("A new 4 x 3 canvas") {
    Canvas canvas(4, 3);

    THEN("It shows opaque black") {
      REQUIRE(canvas.width() == 4);
      REQUIRE(canvas.height() == 3);
      REQUIRE(shown(canvas) == std::vector<uint32_t>(12, 0xFF000000u));
    }

    WHEN("It is filled with a 12-bit colour") {
      canvas.fill(0x5A3);

      THEN("Every pixel takes the fill entry, which holds that colour") {
        REQUIRE(canvas.pixels() == std::vector<uint8_t>(12, FILL));
        REQUIRE(shown(canvas) == std::vector<uint32_t>(12, 0xFF55AA33u));
      }
    }

    WHEN("A palette is set") {
      canvas.fill(0x123);
      canvas.setPalette({0x000, 0xF00, 0x0F0});

      THEN("It fills the first entries and leaves the fill entry alone") {
        REQUIRE(canvas.palette()[1] == 0xF00);
        REQUIRE(canvas.palette()[2] == 0x0F0);
        REQUIRE(canvas.palette()[FILL] == 0x123);
      }
    }
  }
}

SCENARIO("draw pastes a picture's colour numbers") {
  GIVEN("A filled 4 x 3 canvas and a 2 x 2 picture with colour 0") {
    Canvas canvas(4, 3);
    canvas.fill(0x0F0);
    const IndexedBitmap image = picture(2, 2, {0, 1, 1, 0});

    WHEN("It is drawn at 1, 1") {
      canvas.draw(image, 1, 1);

      THEN("Colour 0 is drawn too") {
        REQUIRE(at(canvas, 1, 1) == 0);
        REQUIRE(at(canvas, 2, 1) == 1);
        REQUIRE(at(canvas, 1, 2) == 1);
        REQUIRE(at(canvas, 2, 2) == 0);
        REQUIRE(at(canvas, 0, 0) == FILL);
        REQUIRE(at(canvas, 3, 2) == FILL);
      }
    }

    WHEN("It is drawn across the top-left corner") {
      canvas.draw(image, -1, -1);

      THEN("Only its bottom-right pixel lands") {
        REQUIRE(at(canvas, 0, 0) == 0);
        REQUIRE(at(canvas, 1, 0) == FILL);
        REQUIRE(at(canvas, 0, 1) == FILL);
      }
    }

    WHEN("It is drawn past the bottom-right corner") {
      canvas.draw(image, 3, 2);

      THEN("Only its top-left pixel lands") {
        REQUIRE(at(canvas, 3, 2) == 0);
        REQUIRE(at(canvas, 2, 2) == FILL);
      }
    }
  }

  GIVEN("A picture with its hot spot at 1, 1") {
    Canvas canvas(4, 3);
    const IndexedBitmap image = picture(2, 2, {1, 1, 1, 2}, 1, 1);

    THEN("The hot spot lands on the given point") {
      canvas.draw(image, 2, 2);
      REQUIRE(at(canvas, 1, 1) == 1);
      REQUIRE(at(canvas, 2, 2) == 2);
      REQUIRE(at(canvas, 3, 2) == FILL);
    }
  }

  GIVEN("A drawn picture") {
    Canvas canvas(2, 1);
    canvas.draw(picture(2, 1, {1, 2}), 0, 0);

    THEN("The canvas palette colours it, whenever it is set") {
      canvas.setPalette({0x000, 0xF00, 0x00F});
      REQUIRE(shown(canvas) == std::vector<uint32_t>{0xFFFF0000u, 0xFF0000FFu});
    }
  }
}

SCENARIO("drawMasked pastes a bob") {
  GIVEN("A filled canvas and a 3 x 1 bob with a hole") {
    Canvas canvas(5, 1);
    canvas.fill(0x00F);
    const IndexedBitmap bob = picture(3, 1, {1, 0, 2}, 1, 0);

    WHEN("It is drawn") {
      canvas.drawMasked(bob, 2, 0);

      THEN("Colour 0 lets the canvas through") {
        REQUIRE(at(canvas, 0, 0) == FILL);
        REQUIRE(at(canvas, 1, 0) == 1);
        REQUIRE(at(canvas, 2, 0) == FILL);
        REQUIRE(at(canvas, 3, 0) == 2);
        REQUIRE(at(canvas, 4, 0) == FILL);
      }
    }

    WHEN("It is drawn flipped") {
      canvas.drawMasked(bob, 2, 0, true);

      THEN("The image and its hot spot are mirrored") {
        REQUIRE(at(canvas, 0, 0) == 2);
        REQUIRE(at(canvas, 1, 0) == FILL);
        REQUIRE(at(canvas, 2, 0) == 1);
        REQUIRE(at(canvas, 3, 0) == FILL);
      }
    }
  }
}
