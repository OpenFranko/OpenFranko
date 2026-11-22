#include "../../../../../src/engine/states/shared/FotoScreen.h"

#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace openfranko::src::engine::effects::sequences;
using namespace openfranko::src::engine::states::shared;
using namespace openfranko::src::systems::graphics;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr auto PICTURE = "assets/foto.bmp";
constexpr int WIDTH = 368;
constexpr int HEIGHT = 290;
constexpr int DISPLAY_LINE = 30;
constexpr FotoSequence::Timings TIMINGS{5, 200, 5, 70, true};
constexpr int FADE_FRAMES = 10;

FakeFiles stripedPicture() {
  IndexedBitmap picture;
  picture.width = WIDTH;
  picture.height = HEIGHT;
  picture.pixels.resize(static_cast<std::size_t>(WIDTH) * HEIGHT);
  for (int y = 0; y < HEIGHT; ++y) {
    for (int x = 0; x < WIDTH; ++x) {
      picture.pixels[static_cast<std::size_t>(y * WIDTH + x)] =
          static_cast<uint8_t>(1 + y % 2);
    }
  }
  picture.palette = {0x000, 0x0F0, 0xF00};
  FakeFiles files;
  files.bitmaps[PICTURE] = picture;
  return files;
}

const Layer &picture(const FakeMonitor &monitor) {
  return monitor.shown().layers.front();
}

} // namespace

SCENARIO("A foto screen draws its picture only when it appears or moves") {
  GIVEN("A foto screen whose picture has just appeared") {
    FakeMonitor monitor;
    FakeFiles files = stripedPicture();
    FotoScreen screen(monitor, files, PICTURE, DISPLAY_LINE, TIMINGS);
    do {
      screen.advance();
    } while (!screen.sequence().isShown());
    const uint8_t *shown = picture(monitor).pixels;
    const std::vector<uint16_t> colors = picture(monitor).palette;

    WHEN("Its colours fade over the next frames") {
      std::vector<const uint8_t *> frames;
      for (int frame = 0; frame < FADE_FRAMES; ++frame) {
        screen.advance();
        frames.push_back(picture(monitor).pixels);
      }

      THEN("The same pixels stay shown with the new colours") {
        REQUIRE(frames == std::vector<const uint8_t *>(FADE_FRAMES, shown));
        REQUIRE(picture(monitor).palette != colors);
      }
    }

    WHEN("It shakes") {
      screen.shake();
      screen.advance();
      const uint8_t firstRow = picture(monitor).pixels[0];
      screen.advance();

      THEN("Every frame shows it a line apart from the frame before") {
        REQUIRE(picture(monitor).pixels[0] != firstRow);
      }
    }
  }
}
