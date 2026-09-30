#include "../../../../src/engine/assets/YieldingFiles.h"

#include "FakeFiles.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace openfranko::src::engine::assets;
using openfranko::test::src::engine::assets::FakeFiles;

SCENARIO("YieldingFiles lets the program run after each file it loads") {
  GIVEN("A bitmap and a script behind yielding files") {
    auto fake = std::make_unique<FakeFiles>();
    openfranko::src::systems::graphics::IndexedBitmap picture;
    picture.width = 2;
    picture.height = 1;
    picture.pixels = {1, 2};
    fake->bitmaps["0137/0137_000.bmp"] = picture;
    fake->contents["0385.json"] = {'{', '}'};
    int yields = 0;
    YieldingFiles files(std::move(fake), [&yields] { ++yields; });

    WHEN("Both are loaded") {
      const auto bitmap = files.loadBitmap("0137/0137_000.bmp");
      const auto script = files.read("0385.json");

      THEN("Each load yields once and passes the file on") {
        REQUIRE(yields == 2);
        REQUIRE(bitmap.width == 2);
        REQUIRE(bitmap.pixels == std::vector<uint8_t>{1, 2});
        REQUIRE(script == std::vector<uint8_t>{'{', '}'});
      }
    }

    WHEN("They are only looked up") {
      const bool found = files.exists("0385.json");
      const auto listed = files.list("0137");

      THEN("Nothing yields") {
        REQUIRE(found);
        REQUIRE(listed == std::vector<std::string>{"0137/0137_000.bmp"});
        REQUIRE(yields == 0);
      }
    }
  }
}
