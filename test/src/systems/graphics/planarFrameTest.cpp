#include "../../../../src/systems/graphics/PlanarFrame.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace openfranko::src::systems::graphics;

namespace {

constexpr int WIDTH = 24;
constexpr int HEIGHT = 3;
constexpr int START = 4096;
constexpr int STEPS = 300;
constexpr int SPRITE_VALUE = 0xEE;

uint8_t picture(int column, int row) {
  const uint32_t mixed = static_cast<uint32_t>(column) * 2654435761u ^
                         static_cast<uint32_t>(row) * 40503u;
  return static_cast<uint8_t>(mixed >> 13);
}

IndexedFrame viewed(int offset) {
  IndexedFrame frame;
  frame.width = WIDTH;
  frame.height = HEIGHT;
  for (int row = 0; row < HEIGHT; ++row) {
    for (int x = 0; x < WIDTH; ++x) {
      frame.pixels.push_back(picture(offset + x, row));
    }
  }
  return frame;
}

PlanarFrame split(const IndexedFrame &frame) {
  PlanarFrame planar;
  planar.resize(frame.width, frame.height);
  for (int row = 0; row < frame.height; ++row) {
    planar.copy(frame, row, {0, frame.width});
  }
  return planar;
}

bool matches(const PlanarFrame &planar, const IndexedFrame &frame) {
  for (int row = 0; row < frame.height; ++row) {
    for (int plane = 0; plane < PlanarFrame::PLANES; ++plane) {
      const uint8_t *line = planar.line(plane, row);
      for (int at = 0; at < planar.bytes(); ++at) {
        const std::size_t pixel = static_cast<std::size_t>(
            row * frame.width + at * PlanarFrame::PLANES + plane);
        if (line[at] != frame.pixels[pixel]) {
          return false;
        }
      }
    }
  }
  return true;
}

Span uncovered(int step) {
  if (step >= WIDTH || -step >= WIDTH) {
    return {0, WIDTH};
  }
  return step > 0 ? Span{WIDTH - step, WIDTH} : Span{0, -step};
}

bool pans(const std::vector<int> &steps) {
  int offset = START;
  PlanarFrame planar = split(viewed(offset));
  for (const int step : steps) {
    offset += step;
    const IndexedFrame frame = viewed(offset);
    planar.shift(-step);
    planar.copyColumns(frame, uncovered(step));
    if (!matches(planar, frame)) {
      return false;
    }
  }
  return true;
}

} // namespace

SCENARIO("PlanarFrame splits the pixels of a frame into planes") {
  GIVEN("A frame copied row by row") {
    const IndexedFrame frame = viewed(START);
    const PlanarFrame planar = split(frame);

    THEN("Each plane holds every fourth pixel of each row") {
      REQUIRE(planar.bytes() == WIDTH / PlanarFrame::PLANES);
      REQUIRE(matches(planar, frame));
    }
  }

  GIVEN("A planar frame and a frame changed inside one span") {
    PlanarFrame planar = split(viewed(START));
    IndexedFrame frame = viewed(START);
    const Span sprite{5, 14};
    for (int x = sprite.first; x < sprite.last; ++x) {
      frame.pixels[static_cast<std::size_t>(WIDTH + x)] = SPRITE_VALUE;
    }

    WHEN("Only that span is copied") {
      planar.copy(frame, 1, sprite);

      THEN("The planes hold the changed frame") {
        REQUIRE(matches(planar, frame));
      }
    }
  }

  GIVEN("A planar frame and a frame changed in a few columns of every row") {
    PlanarFrame planar = split(viewed(START));
    IndexedFrame frame = viewed(START);
    const Span columns{9, 12};
    for (int row = 0; row < HEIGHT; ++row) {
      for (int x = columns.first; x < columns.last; ++x) {
        frame.pixels[static_cast<std::size_t>(row * WIDTH + x)] = SPRITE_VALUE;
      }
    }

    WHEN("Only those columns are copied") {
      planar.copyColumns(frame, columns);

      THEN("The planes hold the changed frame") {
        REQUIRE(matches(planar, frame));
      }
    }
  }
}

SCENARIO("PlanarFrame shifts its planes like a panned picture") {
  GIVEN("A planar frame of a picture") {
    WHEN("The picture pans by the same step many times") {
      THEN("Copying only the uncovered columns gives every panned frame") {
        for (const int step : {-9, -5, -4, -3, -2, -1, 1, 2, 3, 4, 5, 9}) {
          REQUIRE(pans(std::vector<int>(STEPS, step)));
        }
      }
    }

    WHEN("The picture pans back and forth") {
      THEN("Copying only the uncovered columns gives every panned frame") {
        std::vector<int> steps;
        for (int count = 0; count < STEPS; ++count) {
          steps.push_back(count % 7 == 0 ? -3 : count % 3 == 0 ? 5 : 1);
        }
        REQUIRE(pans(steps));
      }
    }

    WHEN("The picture pans by a whole frame width or more") {
      THEN("The fully copied frame is kept for later pans") {
        REQUIRE(pans({WIDTH, 1, -WIDTH - 3, -2, 2}));
      }
    }
  }
}
