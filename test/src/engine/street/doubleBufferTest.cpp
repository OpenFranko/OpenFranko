#include "../../../../src/engine/street/DoubleBuffer.h"
#include <catch2/catch_all.hpp>
#include <vector>

using namespace openfranko::src::engine::street;

namespace {

constexpr uint8_t PAPER = 1;
constexpr uint8_t INK = 5;
constexpr uint8_t STAMP = 7;
constexpr uint8_t COPIED = 9;

Picture box(int width, int height, uint8_t color) {
  return Picture{
      width, height, 0, 0,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height), color)};
}

struct Screen {
  ImageBank images;
  BobLayer bobs;
  IndexedSurface paper = IndexedSurface(64, 32);
  DoubleBuffer buffer = DoubleBuffer(paper);

  Screen() {
    images.load(1, {box(16, 8, INK), box(4, 4, STAMP)});
    paper.fill(PAPER);
    buffer = DoubleBuffer(paper);
  }

  void frame() {
    buffer.vbl();
    if (buffer.isAutobacking()) {
      buffer.autobackStep(bobs, images);
    } else {
      buffer.test(bobs, images);
    }
  }

  uint8_t shown(int x, int y) const { return buffer.shown().pixel(x, y); }
};

} // namespace

SCENARIO("Double Buffer shows each automatic update one VBL later") {
  GIVEN("A double-buffered screen with no bobs") {
    Screen screen;

    WHEN("AMAL moves a bob on at a VBL") {
      screen.buffer.vbl();
      screen.bobs.set(1, 16, 8, 1);
      screen.buffer.test(screen.bobs, screen.images);

      THEN("The test draws it into the hidden buffer only") {
        REQUIRE(screen.shown(16, 8) == PAPER);
        REQUIRE(screen.buffer.logic().pixel(16, 8) == PAPER);
      }

      AND_WHEN("The next VBL comes") {
        screen.frame();

        THEN("The swap puts it on screen") {
          REQUIRE(screen.shown(16, 8) == INK);
        }
      }
    }

    WHEN("BASIC sets a bob after the frame's test") {
      screen.frame();
      screen.bobs.set(1, 16, 8, 1);
      screen.frame();
      const uint8_t next = screen.shown(16, 8);
      screen.frame();

      THEN("It is drawn at the next test and shown a VBL after that") {
        REQUIRE(next == PAPER);
        REQUIRE(screen.shown(16, 8) == INK);
      }
    }

    WHEN("Nothing changes") {
      screen.buffer.logic().fill(COPIED);
      screen.frame();
      screen.frame();

      THEN("No update runs, so no swap brings the other buffer up") {
        REQUIRE(screen.shown(0, 0) == PAPER);
      }
    }
  }

  GIVEN("A bob already on screen") {
    Screen screen;
    screen.bobs.set(1, 16, 8, 1);
    screen.frame();
    screen.frame();

    WHEN("It moves 16 pixels right, one VBL at a time") {
      screen.bobs.setX(1, 32);
      screen.frame();
      screen.frame();

      THEN("Neither buffer keeps the old image") {
        REQUIRE(screen.shown(32, 8) == INK);
        REQUIRE(screen.shown(16, 8) == PAPER);
        screen.bobs.setX(1, 48);
        screen.frame();
        screen.frame();
        REQUIRE(screen.shown(48, 8) == INK);
        REQUIRE(screen.shown(32, 8) == PAPER);
        REQUIRE(screen.shown(16, 8) == PAPER);
      }
    }

    WHEN("It is switched off") {
      screen.bobs.off(1);
      screen.frame();
      const uint8_t first = screen.shown(16, 8);
      screen.frame();

      THEN("It goes a VBL later and the hidden buffer never shows it") {
        REQUIRE(first == INK);
        REQUIRE(screen.shown(16, 8) == PAPER);
        screen.frame();
        REQUIRE(screen.shown(16, 8) == PAPER);
      }
    }
  }
}

SCENARIO("Autoback draws into one buffer per VBL and drops the bob flag") {
  GIVEN("A bob on screen and a Paste Bob called during frame 0") {
    Screen screen;
    screen.bobs.set(1, 16, 8, 1);
    screen.frame();
    screen.frame();
    screen.buffer.autoback([&screen](IndexedSurface &surface) {
      BobLayer::paste(surface, screen.images, 40, 20, 2);
    });

    WHEN("The first VBL of the stall comes") {
      screen.frame();

      THEN("The screen still shows the buffer from before the call") {
        REQUIRE(screen.shown(40, 20) == PAPER);
        REQUIRE(screen.shown(16, 8) == INK);
        REQUIRE(screen.buffer.isAutobacking());
      }

      AND_WHEN("The second VBL comes") {
        screen.frame();

        THEN("The stamped buffer is up, with the bob redrawn over it") {
          REQUIRE(screen.shown(40, 20) == STAMP);
          REQUIRE(screen.shown(16, 8) == INK);
        }

        AND_WHEN("The third VBL ends the stall") {
          screen.frame();

          THEN("The other buffer carries the stamp as well") {
            REQUIRE(screen.shown(40, 20) == STAMP);
            REQUIRE_FALSE(screen.buffer.isAutobacking());
          }
        }
      }
    }

    WHEN("AMAL moves the bob at the stall's last VBL") {
      screen.frame();
      screen.frame();
      screen.buffer.vbl();
      screen.bobs.setX(1, 32);
      screen.buffer.autobackStep(screen.bobs, screen.images);
      screen.buffer.test(screen.bobs, screen.images);
      screen.frame();

      THEN("TAbk3's cleared flag leaves the move undrawn") {
        REQUIRE_FALSE(screen.buffer.isDirty(screen.bobs));
        REQUIRE(screen.shown(16, 8) == INK);
        REQUIRE(screen.shown(32, 8) == PAPER);
      }
    }
  }
}

SCENARIO("Bob Update Off leaves drawing and swapping to BASIC") {
  GIVEN("Updates switched off and a bob set") {
    Screen screen;
    screen.buffer.setUpdates(false);
    screen.bobs.set(1, 16, 8, 1);
    screen.frame();
    screen.frame();

    THEN("No automatic update draws it") {
      REQUIRE(screen.shown(16, 8) == PAPER);
    }

    WHEN("Bob Draw and Screen Swap run, then Wait Vbl") {
      screen.buffer.drawBobs(screen.bobs, screen.images);
      screen.buffer.swap();
      const uint8_t before = screen.shown(16, 8);
      screen.frame();

      THEN("The drawn buffer shows from that VBL") {
        REQUIRE(before == PAPER);
        REQUIRE(screen.shown(16, 8) == INK);
      }

      AND_WHEN("Bob Clear runs on the new logic buffer and it is copied into") {
        screen.buffer.clearBobs();
        screen.buffer.logic().fill(COPIED);
        screen.buffer.drawBobs(screen.bobs, screen.images);
        screen.buffer.swap();
        screen.frame();

        THEN("The next pass shows the copy with the bob on top") {
          REQUIRE(screen.shown(0, 0) == COPIED);
          REQUIRE(screen.shown(16, 8) == INK);
        }
      }
    }
  }
}

SCENARIO("Bob Clear restores whole words, even over later writes") {
  GIVEN("A bob drawn at x 20, so its save area spans words 16 to 48") {
    Screen screen;
    screen.buffer.setUpdates(false);
    screen.bobs.set(1, 20, 8, 1);
    screen.buffer.drawBobs(screen.bobs, screen.images);

    WHEN("A logic-only write covers the row, then Bob Clear runs") {
      screen.buffer.logic().clear(COPIED, 0, 8, 64, 9);
      screen.buffer.clearBobs();

      THEN("The saved words come back and the rest keeps the write") {
        REQUIRE(screen.buffer.logic().pixel(16, 8) == PAPER);
        REQUIRE(screen.buffer.logic().pixel(47, 8) == PAPER);
        REQUIRE(screen.buffer.logic().pixel(15, 8) == COPIED);
        REQUIRE(screen.buffer.logic().pixel(48, 8) == COPIED);
      }
    }
  }
}

SCENARIO("Two Screen Swaps in one frame leave the display alone") {
  GIVEN("A frame in which Put Block goes to both buffers") {
    Screen screen;
    screen.buffer.logic().clear(STAMP, 0, 0, 8, 8);
    screen.buffer.swap();
    screen.buffer.logic().clear(STAMP, 0, 0, 8, 8);
    screen.buffer.swap();
    const IndexedSurface *before = &screen.buffer.shown();
    screen.frame();

    THEN("The same buffer stays up and both carry the block") {
      REQUIRE(&screen.buffer.shown() == before);
      REQUIRE(screen.shown(0, 0) == STAMP);
      REQUIRE(screen.buffer.logic().pixel(0, 0) == STAMP);
    }
  }
}
