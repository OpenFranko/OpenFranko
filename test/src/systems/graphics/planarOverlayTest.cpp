#include "../../../../src/systems/graphics/PlanarOverlay.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace openfranko::src::systems::graphics;

namespace {

constexpr int WIDTH = 28;
constexpr int HEIGHT = 6;
constexpr int BYTES = WIDTH / PlanarOverlay::PLANES;

IndexedFrame frameOf() {
  IndexedFrame frame;
  frame.width = WIDTH;
  frame.height = HEIGHT;
  for (int at = 0; at < WIDTH * HEIGHT; ++at) {
    frame.pixels.push_back(static_cast<uint8_t>(at * 7 % 61));
  }
  return frame;
}

Overlay overlayOf(int left, int top, int width, int height) {
  Overlay overlay;
  overlay.left = left;
  overlay.top = top;
  overlay.width = width;
  overlay.height = height;
  overlay.revision = 1;
  for (int at = 0; at < width * height; ++at) {
    overlay.pixels.push_back(static_cast<uint8_t>(100 + at));
    overlay.mask.push_back(at % 3 == 0 ? 0 : 0xFF);
  }
  overlay.shownRows.assign(static_cast<std::size_t>(height), 1);
  return overlay;
}

std::vector<uint8_t> planeRow(const std::vector<uint8_t> &pixels, int plane,
                              int row, std::size_t size) {
  std::vector<uint8_t> line(size, 0);
  for (int at = 0; at < BYTES; ++at) {
    line[static_cast<std::size_t>(at)] = pixels[static_cast<std::size_t>(
        row * WIDTH + at * PlanarOverlay::PLANES + plane)];
  }
  return line;
}

std::vector<uint8_t> composited(const IndexedFrame &frame,
                                const Overlay &overlay) {
  std::vector<uint8_t> pixels = frame.pixels;
  for (int row = 0; row < overlay.height; ++row) {
    for (int x = 0; x < overlay.width; ++x) {
      const std::size_t at = static_cast<std::size_t>(row * overlay.width + x);
      if (overlay.mask[at] != 0) {
        pixels[static_cast<std::size_t>((overlay.top + row) * WIDTH +
                                        overlay.left + x)] = overlay.pixels[at];
      }
    }
  }
  return pixels;
}

bool compositesLike(const IndexedFrame &frame, const Overlay &overlay) {
  PlanarOverlay planar;
  planar.build(overlay);
  const std::size_t size = static_cast<std::size_t>(
      std::max(BYTES, planar.lastWord() * PlanarOverlay::WORD_BYTES));
  const std::vector<uint8_t> expected = composited(frame, overlay);
  for (int row = 0; row < HEIGHT; ++row) {
    for (int plane = 0; plane < PlanarOverlay::PLANES; ++plane) {
      std::vector<uint8_t> line = planeRow(frame.pixels, plane, row, size);
      planar.composite(plane, row, line.data());
      if (line != planeRow(expected, plane, row, size)) {
        return false;
      }
    }
  }
  return true;
}

} // namespace

SCENARIO("PlanarOverlay draws an overlay over the planes of a frame") {
  GIVEN("A frame and overlays at every alignment") {
    const IndexedFrame frame = frameOf();

    THEN("Each plane row shows the overlay where its mask is set") {
      for (int left = 0; left < 8; ++left) {
        for (const int width : {1, 3, 4, 9, 13}) {
          REQUIRE(compositesLike(frame, overlayOf(left, 1, width, 4)));
        }
      }
    }
  }

  GIVEN("An overlay that reaches the right edge of the frame") {
    const IndexedFrame frame = frameOf();

    THEN("The planes show it up to the edge") {
      REQUIRE(compositesLike(frame, overlayOf(WIDTH - 9, 0, 9, HEIGHT)));
    }
  }

  GIVEN("A built overlay") {
    Overlay overlay = overlayOf(5, 1, 11, 3);
    PlanarOverlay planar;
    planar.build(overlay);

    THEN("It is up to date until the overlay changes") {
      REQUIRE(planar.isBuiltFrom(overlay));
      ++overlay.revision;
      REQUIRE_FALSE(planar.isBuiltFrom(overlay));
    }
  }
}
