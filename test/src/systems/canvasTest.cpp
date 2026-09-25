#include "../../../src/systems/Canvas.h"
#include <catch2/catch_all.hpp>

#include <cstdint>
#include <vector>

using namespace openfranko::src::systems;

namespace {

constexpr uint32_t BLACK = 0xFF000000u;
constexpr uint32_t RED = 0xFFFF0000u;
constexpr uint32_t GREEN = 0xFF00FF00u;
constexpr uint32_t BLUE = 0xFF0000FFu;
const std::vector<uint16_t> PALETTE = {0x000, 0xF00, 0x0F0};

IndexedBitmap picture(int width, int height, std::vector<uint8_t> pixels,
                      int hotX = 0, int hotY = 0) {
  return IndexedBitmap{width, height, hotX, hotY, std::move(pixels), PALETTE};
}

uint32_t at(const Canvas &canvas, int x, int y) {
  return canvas.pixels()[static_cast<std::size_t>(y * canvas.width() + x)];
}

} // namespace

SCENARIO("A canvas holds one opaque ARGB frame") {
  GIVEN("A new 4 x 3 canvas") {
    Canvas canvas(4, 3);

    THEN("It starts opaque black") {
      REQUIRE(canvas.width() == 4);
      REQUIRE(canvas.height() == 3);
      REQUIRE(canvas.pixels() == std::vector<uint32_t>(12, BLACK));
    }

    WHEN("It is filled with a 12-bit colour") {
      canvas.fill(0x5A3);

      THEN("Every channel is the nibble times 17 at full alpha") {
        REQUIRE(canvas.pixels() == std::vector<uint32_t>(12, 0xFF55AA33u));
      }
    }
  }
}

SCENARIO("draw pastes a picture through a palette") {
  GIVEN("A 4 x 3 canvas filled green and a 2 x 2 picture with colour 0") {
    Canvas canvas(4, 3);
    canvas.fill(0x0F0);
    const IndexedBitmap image = picture(2, 2, {0, 1, 1, 0});

    WHEN("It is drawn at 1, 1") {
      canvas.draw(image, PALETTE, 1, 1);

      THEN("Colour 0 is drawn too") {
        REQUIRE(at(canvas, 1, 1) == BLACK);
        REQUIRE(at(canvas, 2, 1) == RED);
        REQUIRE(at(canvas, 1, 2) == RED);
        REQUIRE(at(canvas, 2, 2) == BLACK);
        REQUIRE(at(canvas, 0, 0) == GREEN);
        REQUIRE(at(canvas, 3, 2) == GREEN);
      }
    }

    WHEN("It is drawn across the top-left corner") {
      canvas.draw(image, PALETTE, -1, -1);

      THEN("Only its bottom-right pixel lands, and nothing else changes") {
        REQUIRE(at(canvas, 0, 0) == BLACK);
        REQUIRE(at(canvas, 1, 0) == GREEN);
        REQUIRE(at(canvas, 0, 1) == GREEN);
      }
    }

    WHEN("It is drawn past the bottom-right corner") {
      canvas.draw(image, PALETTE, 3, 2);

      THEN("Only its top-left pixel lands") {
        REQUIRE(at(canvas, 3, 2) == BLACK);
        REQUIRE(at(canvas, 2, 2) == GREEN);
      }
    }

    WHEN("It is drawn with another palette") {
      canvas.draw(image, {0x00F, 0xFFF}, 1, 1);

      THEN("That palette colours it") {
        REQUIRE(at(canvas, 1, 1) == BLUE);
        REQUIRE(at(canvas, 2, 1) == 0xFFFFFFFFu);
      }
    }
  }

  GIVEN("A picture with its hot spot at 1, 1") {
    Canvas canvas(4, 3);
    const IndexedBitmap image = picture(2, 2, {1, 1, 1, 2}, 1, 1);

    THEN("The hot spot lands on the given point") {
      canvas.draw(image, PALETTE, 2, 2);
      REQUIRE(at(canvas, 1, 1) == RED);
      REQUIRE(at(canvas, 2, 2) == GREEN);
      REQUIRE(at(canvas, 3, 2) == BLACK);
    }
  }
}

SCENARIO("drawMasked pastes a bob") {
  GIVEN("A blue canvas and a 3 x 1 bob with a hole") {
    Canvas canvas(5, 1);
    canvas.fill(0x00F);
    const IndexedBitmap bob = picture(3, 1, {1, 0, 2}, 1, 0);

    WHEN("It is drawn") {
      canvas.drawMasked(bob, PALETTE, 2, 0);

      THEN("Colour 0 lets the canvas through") {
        REQUIRE(at(canvas, 0, 0) == BLUE);
        REQUIRE(at(canvas, 1, 0) == RED);
        REQUIRE(at(canvas, 2, 0) == BLUE);
        REQUIRE(at(canvas, 3, 0) == GREEN);
        REQUIRE(at(canvas, 4, 0) == BLUE);
      }
    }

    WHEN("It is drawn flipped") {
      canvas.drawMasked(bob, PALETTE, 2, 0, true);

      THEN("The image and its hot spot are mirrored") {
        REQUIRE(at(canvas, 0, 0) == GREEN);
        REQUIRE(at(canvas, 1, 0) == BLUE);
        REQUIRE(at(canvas, 2, 0) == RED);
        REQUIRE(at(canvas, 3, 0) == BLUE);
      }
    }
  }
}
