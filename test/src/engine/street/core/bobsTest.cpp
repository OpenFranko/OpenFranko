#include "../../../../../src/engine/street/core/Bobs.h"

#include "box.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

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
constexpr int MAX_WORDS = 3;
constexpr int MAX_HEIGHT = 24;
constexpr int WORD_BITS = 16;
constexpr uint16_t ORIENTATIONS[] = {0, MIRROR, UPSIDE_DOWN,
                                     MIRROR | UPSIDE_DOWN};

int uniform(std::mt19937 &random, int low, int high) {
  return std::uniform_int_distribution<int>(low, high)(random);
}

Picture randomMask(std::mt19937 &random, int words) {
  Picture picture =
      box(WORD_BITS * words, uniform(random, 1, MAX_HEIGHT), 0, 0, 0);
  picture.hotX = uniform(random, 0, picture.width);
  picture.hotY = uniform(random, 0, picture.height);
  const int solidOneIn = uniform(random, 1, 8);
  for (uint8_t &pixel : picture.pixels) {
    pixel = uniform(random, 1, solidOneIn) == 1 ? 9 : 0;
  }
  return picture;
}

struct BlitMask {
  int left = 0;
  int top = 0;
  int words = 0;
  int height = 0;
  std::vector<uint16_t> data;
};

int hotSpotX(const Picture &picture, uint16_t orientation) {
  return orientation & MIRROR ? picture.width - picture.hotX : picture.hotX;
}

int hotSpotY(const Picture &picture, uint16_t orientation) {
  return orientation & UPSIDE_DOWN ? picture.height - picture.hotY
                                   : picture.hotY;
}

BlitMask blitMask(const Picture &picture, uint16_t orientation, int x, int y) {
  BlitMask mask;
  mask.left = x - hotSpotX(picture, orientation);
  mask.top = y - hotSpotY(picture, orientation);
  mask.words = picture.width / WORD_BITS;
  mask.height = picture.height;
  for (int row = 0; row < picture.height; ++row) {
    const int sourceRow =
        orientation & UPSIDE_DOWN ? picture.height - 1 - row : row;
    for (int word = 0; word < mask.words; ++word) {
      uint16_t bits = 0;
      for (int bit = 0; bit < WORD_BITS; ++bit) {
        const int column = word * WORD_BITS + bit;
        const int sourceColumn =
            orientation & MIRROR ? picture.width - 1 - column : column;
        const bool solid = picture.pixels[static_cast<std::size_t>(
                               sourceRow * picture.width + sourceColumn)] != 0;
        bits = static_cast<uint16_t>(bits << 1 | (solid ? 1 : 0));
      }
      mask.data.push_back(bits);
    }
  }
  return mask;
}

uint16_t maskWord(const BlitMask &mask, int index) {
  return static_cast<std::size_t>(index) < mask.data.size()
             ? mask.data[static_cast<std::size_t>(index)]
             : 0;
}

bool colRout(const BlitMask &tested, const BlitMask &other) {
  const bool testedOnRight = tested.left >= other.left;
  const BlitMask &a = testedOnRight ? other : tested;
  const BlitMask &b = testedOnRight ? tested : other;
  const int aRight = a.left + a.words * WORD_BITS;
  const int bRight = b.left + b.words * WORD_BITS;
  const int top = std::max(a.top, b.top);
  const int bottom = std::min(a.top + a.height, b.top + b.height);
  if (b.left >= aRight || top >= bottom) {
    return false;
  }
  const int dx = b.left - a.left;
  const int shift = dx % WORD_BITS;
  const int width =
      (std::min(aRight, bRight) - b.left) / WORD_BITS + (shift != 0 ? 1 : 0);
  uint16_t previous = 0;
  for (int y = top; y < bottom; ++y) {
    for (int word = 0; word < width; ++word) {
      uint16_t aWord =
          maskWord(a, (y - a.top) * a.words + dx / WORD_BITS + word);
      if (word == 0) {
        aWord &= static_cast<uint16_t>(0xFFFF >> shift);
      }
      const uint16_t bWord = maskWord(b, (y - b.top) * b.words + word);
      const uint16_t shifted = static_cast<uint16_t>(
          (static_cast<uint32_t>(previous) << WORD_BITS | bWord) >> shift);
      previous = bWord;
      if ((aWord & shifted) != 0) {
        return true;
      }
    }
  }
  return false;
}

bool bitAt(const BlitMask &mask, int x, int y) {
  const int column = x - mask.left;
  const int row = y - mask.top;
  if (column < 0 || row < 0 || column >= mask.words * WORD_BITS ||
      row >= mask.height) {
    return false;
  }
  const uint16_t word = mask.data[static_cast<std::size_t>(row * mask.words +
                                                           column / WORD_BITS)];
  return (word >> (WORD_BITS - 1 - column % WORD_BITS) & 1) != 0;
}

bool bitsMeet(const BlitMask &first, const BlitMask &second) {
  for (int y = first.top; y < first.top + first.height; ++y) {
    for (int x = first.left; x < first.left + first.words * WORD_BITS; ++x) {
      if (bitAt(first, x, y) && bitAt(second, x, y)) {
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

SCENARIO("Bob Col reads one word past a narrow bob, like the AMOS blitter") {
  GIVEN("A 48 x 2 bob solid only at (21, 0) and a 16 x 2 bob with a solid "
        "second row") {
    ImageBank images;
    Picture wide = box(48, 2, 0, 0, 0);
    wide.pixels[21] = 5;
    Picture narrow = box(16, 2, 0, 0, 0);
    std::fill(narrow.pixels.begin() + 16, narrow.pixels.end(), 5);
    images.load(1, {wide, narrow});
    BobLayer bobs;
    bobs.set(1, 0, 0, 1);

    THEN("5 pixels off the word grid, its second row is read at x 21 and "
         "collides") {
      bobs.set(2, 5, 0, 2);
      REQUIRE(bobs.collide(1, images));
      REQUIRE(bobs.collide(2, images));
    }

    THEN("On the word grid no extra word is read") {
      bobs.set(2, 0, 0, 2);
      REQUIRE_FALSE(bobs.collide(1, images));
      REQUIRE_FALSE(bobs.collide(2, images));
    }

    THEN("Past the narrow bob's last row the extra word finds nothing") {
      bobs.set(1, 0, 1, 1);
      bobs.set(2, 5, 0, 2);
      REQUIRE_FALSE(bobs.collide(1, images));
      REQUIRE_FALSE(bobs.collide(2, images));
    }
  }
}

SCENARIO("Bob Col agrees with the AMOS blitter test in every orientation") {
  GIVEN("Random masks, hot spots and orientations") {
    std::mt19937 random(2026);
    const auto check = [&random](const Picture &first, const Picture &second,
                                 int left, int top, int &spills) {
      const uint16_t firstOrientation = ORIENTATIONS[uniform(random, 0, 3)];
      const uint16_t secondOrientation = ORIENTATIONS[uniform(random, 0, 3)];
      const int firstX = hotSpotX(first, firstOrientation);
      const int firstY = hotSpotY(first, firstOrientation);
      const int secondX = left + hotSpotX(second, secondOrientation);
      const int secondY = top + hotSpotY(second, secondOrientation);
      ImageBank images;
      images.load(1, {first, second});
      images.orient(1, firstOrientation);
      images.orient(2, secondOrientation);
      BobLayer bobs;
      bobs.set(1, firstX, firstY, 1);
      bobs.set(2, secondX, secondY, 2);
      const BlitMask firstMask =
          blitMask(first, firstOrientation, firstX, firstY);
      const BlitMask secondMask =
          blitMask(second, secondOrientation, secondX, secondY);
      const bool expected = colRout(firstMask, secondMask);
      REQUIRE(bobs.collide(1, images) == expected);
      REQUIRE(bobs.collide(2, images) == colRout(secondMask, firstMask));
      spills += expected && !bitsMeet(firstMask, secondMask) ? 1 : 0;
      return expected;
    };

    THEN("Two bobs anywhere collide exactly when the blitter finds a common "
         "bit") {
      int collisions = 0;
      int spills = 0;
      for (int trial = 0; trial < RANDOM_TRIALS; ++trial) {
        CAPTURE(trial);
        const Picture first = randomMask(random, uniform(random, 1, MAX_WORDS));
        const Picture second =
            randomMask(random, uniform(random, 1, MAX_WORDS));
        const bool collided = check(
            first, second, uniform(random, -MAX_WORDS * WORD_BITS, first.width),
            uniform(random, -MAX_HEIGHT, first.height), spills);
        collisions += collided ? 1 : 0;
      }
      REQUIRE(collisions > RANDOM_TRIALS / 10);
      REQUIRE(collisions < RANDOM_TRIALS * 9 / 10);
    }

    THEN("A narrow bob inside a wide one off the word grid also collides "
         "where the blitter reads its next row") {
      int spills = 0;
      for (int trial = 0; trial < RANDOM_TRIALS; ++trial) {
        CAPTURE(trial);
        const Picture wide = randomMask(random, MAX_WORDS);
        const Picture narrow = randomMask(random, 1);
        const int left =
            WORD_BITS * uniform(random, 0, 1) + uniform(random, 1, 15);
        const int top = uniform(random, 1 - narrow.height, wide.height - 1);
        check(wide, narrow, left, top, spills);
      }
      REQUIRE(spills > RANDOM_TRIALS / 100);
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
