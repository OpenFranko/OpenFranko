#include "../../../../src/systems/jaguar/ObjectList.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>

using namespace openfranko::src::systems::jaguar;

namespace {

constexpr uint32_t LIVE_ADDRESS = 0x1010;

uint32_t type(uint64_t phrase) { return static_cast<uint32_t>(phrase & 7); }

uint32_t link(uint64_t phrase) {
  return static_cast<uint32_t>((phrase >> 24) & 0x7FFFF) << 3;
}

BitmapObject bitmap(bool scaled) {
  BitmapObject object;
  object.data = 0x20000;
  object.y = 80;
  object.height = 10;
  object.dataWidth = 40;
  object.imageWidth = 40;
  object.scaled = scaled;
  return object;
}

} // namespace

SCENARIO("An object list pads bitmaps to their alignment and links past the "
         "padding") {
  GIVEN("A branch, a bitmap, a scaled bitmap and a stop in a list that "
        "needs padding before each bitmap") {
    ObjectList list(LIVE_ADDRESS);
    list.addBranch(100, Branch::Below, 3);
    const std::size_t plain = list.addBitmap(bitmap(false));
    const std::size_t scaled = list.addBitmap(bitmap(true));
    const std::size_t stop = list.addStop();

    THEN("Each bitmap starts on its boundary") {
      REQUIRE(list.address(plain) % OBJECT_ALIGNMENT == 0);
      REQUIRE(list.address(scaled) % SCALED_ALIGNMENT == 0);
      REQUIRE(plain > 1);
      REQUIRE(scaled > plain + 2);
    }

    THEN("The size counts every phrase, the padding included") {
      REQUIRE(list.size() == stop + 1);
      REQUIRE(list.size() == list.phrases().size());
    }

    THEN("Each bitmap links straight to the next object") {
      REQUIRE(link(list.phrases()[plain]) == list.address(scaled));
      REQUIRE(link(list.phrases()[scaled]) == list.address(stop));
      REQUIRE(type(list.phrases()[stop]) == 4);
    }

    THEN("The padding is branches that are never taken") {
      for (const std::size_t at : {plain - 1, scaled - 2, scaled - 1}) {
        REQUIRE(type(list.phrases()[at]) == 3);
        REQUIRE(((list.phrases()[at] >> 3) & 0x7FF) == 0x7FE);
      }
      REQUIRE(scaled - 2 == plain + 2);
    }
  }

  GIVEN("A list moved to another live address") {
    ObjectList list(LIVE_ADDRESS);
    list.addBranch(100, Branch::Below, 1);
    list.addStop();
    list.reset(0x4000);

    THEN("It starts empty and its branches point into the new place") {
      REQUIRE(list.size() == 0);
      list.addBranch(100, Branch::Above, 2);
      REQUIRE(list.size() == 1);
      REQUIRE(link(list.phrases()[0]) == 0x4000 + 2 * PHRASE_BYTES);
    }
  }
}
