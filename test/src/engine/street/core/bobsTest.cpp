#include "../../../../../src/engine/street/core/Bobs.h"

#include "box.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <random>

using namespace openfranko::src::engine::street::core;
using namespace openfranko::test::src::engine::street::core;

namespace {

constexpr int MIRROR = 0x8000;
constexpr int UPSIDE_DOWN = 0x4000;

Picture leftEdge(int width, int hotX) {
  Picture picture = box(width, 1, hotX, 0, 0);
  picture.pixels[0] = 7;
  return picture;
}

constexpr int RANDOM_TRIALS = 4000;
constexpr int MAX_SIDE = 24;
constexpr uint16_t ORIENTATIONS[] = {0, MIRROR, UPSIDE_DOWN,
                                     MIRROR | UPSIDE_DOWN};

int uniform(std::mt19937 &random, int low, int high) {
  return std::uniform_int_distribution<int>(low, high)(random);
}

Picture randomMask(std::mt19937 &random) {
  Picture picture =
      box(uniform(random, 1, MAX_SIDE), uniform(random, 1, MAX_SIDE), 0, 0, 0);
  picture.hotX = uniform(random, 0, picture.width);
  picture.hotY = uniform(random, 0, picture.height);
  const int solidOneIn = uniform(random, 1, 8);
  for (uint8_t &pixel : picture.pixels) {
    pixel = uniform(random, 1, solidOneIn) == 1 ? 9 : 0;
  }
  return picture;
}

struct Placed {
  const Picture *picture = nullptr;
  uint16_t orientation = 0;
  int left = 0;
  int top = 0;
};

Placed place(const Picture &picture, uint16_t orientation, int x, int y) {
  const int hotX =
      orientation & MIRROR ? picture.width - picture.hotX : picture.hotX;
  const int hotY =
      orientation & UPSIDE_DOWN ? picture.height - picture.hotY : picture.hotY;
  return Placed{&picture, orientation, x - hotX, y - hotY};
}

bool solidAt(const Placed &bob, int x, int y) {
  const Picture &picture = *bob.picture;
  int column = x - bob.left;
  int row = y - bob.top;
  if (column < 0 || row < 0 || column >= picture.width ||
      row >= picture.height) {
    return false;
  }
  if (bob.orientation & MIRROR) {
    column = picture.width - 1 - column;
  }
  if (bob.orientation & UPSIDE_DOWN) {
    row = picture.height - 1 - row;
  }
  return picture
             .pixels[static_cast<std::size_t>(row * picture.width + column)] !=
         0;
}

bool solidPixelsMeet(const Placed &first, const Placed &second) {
  for (int y = first.top; y < first.top + first.picture->height; ++y) {
    for (int x = first.left; x < first.left + first.picture->width; ++x) {
      if (solidAt(first, x, y) && solidAt(second, x, y)) {
        return true;
      }
    }
  }
  return false;
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

  GIVEN("An image whose only solid pixel is its top left corner") {
    ImageBank images;
    Picture corner = box(16, 10, 0, 0, 0);
    corner.pixels[0] = 7;
    images.load(5, {corner, box(1, 1, 0, 0, 1)});
    BobLayer bobs;
    IndexedSurface screen(320, 222);
    bobs.set(1, 100, 50, 5 + UPSIDE_DOWN);
    bobs.draw(screen, images);

    THEN("Drawn upside down, its pixel is on the box's bottom row") {
      bobs.set(2, 100, 49, 6);
      REQUIRE(bobs.collide(1, images));
      bobs.set(2, 100, 40, 6);
      REQUIRE_FALSE(bobs.collide(1, images));
    }

    WHEN("It is drawn mirrored as well") {
      bobs.setImage(1, 5 + MIRROR + UPSIDE_DOWN);
      bobs.draw(screen, images);

      THEN("Its pixel is in the bottom right corner") {
        bobs.set(2, 99, 49, 6);
        REQUIRE(bobs.collide(1, images));
        bobs.set(2, 84, 49, 6);
        REQUIRE_FALSE(bobs.collide(1, images));
      }
    }
  }
}

SCENARIO("No Mask makes an image opaque and blind to Bob Col") {
  GIVEN(
      "Two solid images with a hole in the corner, the first set to No Mask") {
    ImageBank images;
    Picture holed = box(8, 4, 0, 0, 6);
    holed.pixels[0] = 0;
    images.load(1, {holed, holed});
    images.noMask(1);
    IndexedSurface screen(320, 222);
    screen.fill(9);

    THEN("Paste Bob writes its colour 0 as well") {
      REQUIRE(BobLayer::paste(screen, images, 10, 20, 1));
      REQUIRE(screen.pixel(10, 20) == 0);
      REQUIRE(screen.pixel(11, 20) == 6);
      REQUIRE(BobLayer::paste(screen, images, 30, 20, 2));
      REQUIRE(screen.pixel(30, 20) == 9);
    }

    THEN("A bob showing it covers what is behind") {
      BobLayer bobs;
      bobs.set(1, 10, 20, 1);
      bobs.draw(screen, images);
      REQUIRE(screen.pixel(10, 20) == 0);
    }

    THEN("It never collides, from either side") {
      BobLayer bobs;
      bobs.set(1, 0, 0, 1);
      bobs.set(2, 4, 2, 2);
      REQUIRE_FALSE(bobs.collide(1, images));
      REQUIRE_FALSE(bobs.collide(2, images));
      bobs.setImage(1, 2);
      REQUIRE(bobs.collide(2, images));
    }

    THEN("Loading the bank again brings the mask back") {
      REQUIRE_FALSE(images.isMasked(1));
      REQUIRE(images.isMasked(2));
      images.load(1, {holed});
      REQUIRE(images.isMasked(1));
    }
  }
}

SCENARIO("Bob Col agrees with a pixel by pixel test in every orientation") {
  GIVEN("Random masks, hot spots, orientations and positions") {
    std::mt19937 random(2026);

    THEN("Two bobs collide exactly when solid pixels share a screen pixel") {
      int collisions = 0;
      for (int trial = 0; trial < RANDOM_TRIALS; ++trial) {
        const Picture first = randomMask(random);
        const Picture second = randomMask(random);
        const uint16_t firstOrientation = ORIENTATIONS[uniform(random, 0, 3)];
        const uint16_t secondOrientation = ORIENTATIONS[uniform(random, 0, 3)];
        const int x = uniform(random, -MAX_SIDE, MAX_SIDE);
        const int y = uniform(random, -MAX_SIDE, MAX_SIDE);
        ImageBank images;
        images.load(1, {first, second});
        images.orient(1, firstOrientation);
        images.orient(2, secondOrientation);
        BobLayer bobs;
        bobs.set(1, 0, 0, 1);
        bobs.set(2, x, y, 2);
        const bool expected =
            solidPixelsMeet(place(first, firstOrientation, 0, 0),
                            place(second, secondOrientation, x, y));
        CAPTURE(trial);
        REQUIRE(bobs.collide(1, images) == expected);
        REQUIRE(bobs.collide(2, images) == expected);
        collisions += expected ? 1 : 0;
      }
      REQUIRE(collisions > RANDOM_TRIALS / 10);
      REQUIRE(collisions < RANDOM_TRIALS * 9 / 10);
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
