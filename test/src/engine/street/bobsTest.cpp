#include "../../../../src/engine/street/Bobs.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::src::engine::street;

namespace {

constexpr int MIRROR = 0x8000;
constexpr int UPSIDE_DOWN = 0x4000;

Picture box(int width, int height, int hotX, int hotY, uint8_t color) {
  return Picture{
      width, height, hotX, hotY,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height), color)};
}

Picture leftEdge(int width, int hotX) {
  Picture picture = box(width, 1, hotX, 0, 0);
  picture.pixels[0] = 7;
  return picture;
}

} // namespace

SCENARIO("A bob is placed by its hot spot") {
  GIVEN("A 32 x 10 image with its hot spot at (10, 9)") {
    ImageBank images;
    Picture picture = box(32, 10, 10, 9, 3);
    picture.pixels[0] = 4;
    images.load(1, {picture});
    BobLayer bobs;
    IndexedSurface screen(320, 222);

    WHEN("It is drawn at (100, 100)") {
      bobs.set(1, 100, 100, 1);
      bobs.draw(screen, images);

      THEN("Its top-left corner lands at (90, 91)") {
        REQUIRE(screen.pixel(90, 91) == 4);
        REQUIRE(screen.pixel(89, 91) == 0);
        REQUIRE(screen.pixel(90, 90) == 0);
      }
    }

    WHEN("It is drawn mirrored") {
      bobs.set(1, 100, 100, 1 + MIRROR);
      bobs.draw(screen, images);

      THEN("The hot spot is w - hx, so the corner lands at (78, 91)") {
        REQUIRE(screen.pixel(78, 91) == 3);
        REQUIRE(screen.pixel(109, 91) == 4);
        REQUIRE(screen.pixel(77, 91) == 0);
      }
    }

    WHEN("It is drawn upside down") {
      bobs.set(1, 100, 100, 1 + UPSIDE_DOWN);
      bobs.draw(screen, images);

      THEN("The hot spot is h - hy, so the image starts at row 99") {
        REQUIRE(screen.pixel(90, 108) == 4);
        REQUIRE(screen.pixel(90, 98) == 0);
      }
    }
  }
}

SCENARIO("Priority On draws lower bobs in front") {
  GIVEN("Two overlapping bobs, the lower one numbered first") {
    ImageBank images;
    images.load(1, {box(16, 16, 0, 15, 1), box(16, 16, 0, 15, 2)});
    BobLayer bobs;
    IndexedSurface screen(320, 222);

    THEN("The one with the greater Y wins, whatever its number") {
      bobs.set(1, 50, 110, 1);
      bobs.set(2, 50, 100, 2);
      bobs.draw(screen, images);
      REQUIRE(screen.pixel(55, 100) == 1);
    }

    THEN("On equal Y the greater X wins, then the higher number") {
      bobs.set(1, 58, 100, 1);
      bobs.set(2, 50, 100, 2);
      bobs.draw(screen, images);
      REQUIRE(screen.pixel(60, 90) == 1);
      bobs.set(1, 50, 100, 1);
      bobs.draw(screen, images);
      REQUIRE(screen.pixel(55, 90) == 2);
    }
  }
}

SCENARIO("Paste Bob stamps by the top-left corner") {
  GIVEN("A screen and an image with a hot spot") {
    ImageBank images;
    Picture picture = box(8, 4, 5, 3, 6);
    picture.pixels[1] = 0;
    images.load(1, {picture});
    IndexedSurface screen(320, 222);

    THEN("The hot spot is ignored and colour 0 stays transparent") {
      screen.fill(9);
      REQUIRE(BobLayer::paste(screen, images, 10, 20, 1));
      REQUIRE(screen.pixel(10, 20) == 6);
      REQUIRE(screen.pixel(11, 20) == 9);
      REQUIRE(screen.pixel(9, 20) == 9);
    }

    THEN("A stamp wholly off the screen draws nothing and reports it") {
      REQUIRE_FALSE(BobLayer::paste(screen, images, 400, 20, 1));
      REQUIRE_FALSE(BobLayer::paste(screen, images, 10, 20, 2));
    }

    THEN("The mirror bit flips the stamp") {
      REQUIRE(BobLayer::paste(screen, images, 10, 20, 1 + MIRROR));
      REQUIRE(screen.pixel(16, 20) == 0);
      REQUIRE(screen.pixel(17, 20) == 6);
    }
  }
}

SCENARIO("Bob Col tests masks inside strictly overlapping boxes") {
  GIVEN("Solid 16 x 10 images and one with a hole") {
    ImageBank images;
    Picture holed = box(16, 10, 0, 0, 1);
    holed.pixels[5 * 16 + 15] = 0;
    images.load(1, {box(16, 10, 0, 0, 1), box(1, 1, 0, 0, 1), holed});
    BobLayer bobs;

    THEN("Boxes that only touch do not collide") {
      bobs.set(1, 0, 0, 1);
      bobs.set(2, 16, 0, 1);
      REQUIRE_FALSE(bobs.collide(1, images));
      bobs.set(2, 15, 0, 1);
      REQUIRE(bobs.collide(1, images));
      REQUIRE(bobs.collided(2));
      REQUIRE_FALSE(bobs.collided(1));
    }

    THEN("A transparent pixel does not collide") {
      bobs.set(1, 0, 0, 3);
      bobs.set(2, 15, 5, 2);
      REQUIRE_FALSE(bobs.collide(1, images));
      bobs.set(2, 15, 4, 2);
      REQUIRE(bobs.collide(1, images));
    }

    THEN("Only the asked range is tested, and hidden bobs never collide") {
      bobs.set(1, 0, 0, 1);
      bobs.set(2, 5, 5, 1);
      bobs.set(3, 6, 6, 1);
      REQUIRE(bobs.collide(1, images, 3, 3));
      REQUIRE_FALSE(bobs.collided(2));
      REQUIRE(bobs.collided(3));
      bobs.off(3);
      bobs.set(2, 5, 5, 0);
      REQUIRE_FALSE(bobs.collide(1, images));
      bobs.set(2, 5, 5, 9);
      REQUIRE_FALSE(bobs.collide(1, images));
    }
  }
}

SCENARIO("Bob Col sees an image as it was last drawn") {
  GIVEN("An image whose only solid pixel is its left edge, hot spot centred") {
    ImageBank images;
    images.load(5, {leftEdge(16, 8), box(1, 1, 0, 0, 1)});
    BobLayer bobs;
    IndexedSurface screen(320, 222);
    bobs.set(1, 100, 0, 5);
    bobs.set(2, 107, 0, 6);

    THEN("Unmirrored its pixel is at 92, clear of the probe at 107") {
      REQUIRE_FALSE(bobs.collide(1, images));
    }

    WHEN("The bob is mirrored but not yet redrawn") {
      bobs.setImage(1, 5 + MIRROR);

      THEN("AMOS has not flipped the image yet, so it still misses") {
        REQUIRE_FALSE(bobs.collide(1, images));
      }

      AND_WHEN("The frame is drawn") {
        bobs.draw(screen, images);

        THEN("The flipped image puts its pixel on 107 and it collides") {
          REQUIRE(images.orientation(5) == MIRROR);
          REQUIRE(bobs.collide(1, images));
        }

        AND_WHEN("It is turned back without a redraw") {
          bobs.setImage(1, 5);

          THEN("The image is still flipped in the bank") {
            REQUIRE(bobs.collide(1, images));
          }
        }
      }
    }

    WHEN("It is mirrored while wholly off the screen and drawn") {
      bobs.set(1, 1000, 0, 5 + MIRROR);
      bobs.draw(screen, images);

      THEN("An undrawn bob does not flip its image") {
        REQUIRE(images.orientation(5) == 0);
      }
    }
  }
}

SCENARIO("The sprite bank holds frames at base plus index") {
  GIVEN("Two sets loaded over each other") {
    ImageBank images;
    images.load(1, {box(16, 2, 0, 0, 1), box(16, 2, 0, 0, 2)});
    images.load(2, {box(8, 2, 0, 0, 3), Picture{}});

    THEN("The later set wins, and an empty frame leaves no image") {
      REQUIRE(images.find(1)->pixels[0] == 1);
      REQUIRE(images.find(2)->pixels[0] == 3);
      REQUIRE(images.find(3) == nullptr);
      REQUIRE(images.find(0) == nullptr);
    }
  }
}
