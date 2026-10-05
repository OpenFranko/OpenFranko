#include "../../../../src/engine/assets/YieldingFiles.h"

#include "FakeFiles.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
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

    WHEN("Both are read in steps") {
      const auto bitmapLoad = files.beginBitmap("0137/0137_000.bmp");
      openfranko::src::systems::graphics::IndexedBitmap bitmap;
      const bool bitmapDone = bitmapLoad->step(bitmap);
      const auto scriptLoad = files.beginRead("0385.json");
      std::vector<uint8_t> script;
      const bool scriptDone = scriptLoad->step(script);

      THEN("The steps pass the files on without yielding") {
        REQUIRE(bitmapDone);
        REQUIRE(scriptDone);
        REQUIRE(bitmap.pixels == std::vector<uint8_t>{1, 2});
        REQUIRE(script == std::vector<uint8_t>{'{', '}'});
        REQUIRE(yields == 0);
      }
    }

    WHEN("They are only looked up") {
      const bool found = files.exists("0385.json");
      const auto listed = files.list("0137");
      const auto walk = files.walk("0137");
      std::vector<std::string> walked;
      std::string_view name;
      while (walk->next(name)) {
        walked.emplace_back(name);
      }

      THEN("Nothing yields") {
        REQUIRE(found);
        REQUIRE(listed == std::vector<std::string>{"0137/0137_000.bmp"});
        REQUIRE(walked == std::vector<std::string>{"0137_000.bmp"});
        REQUIRE(yields == 0);
      }
    }
  }
}

SCENARIO("An error raised while yielding reaches the loader") {
  GIVEN("Yielding files whose yield fails") {
    auto fake = std::make_unique<FakeFiles>();
    fake->bitmaps["0137/0137_000.bmp"] = {};
    fake->contents["0385.json"] = {'{', '}'};
    FakeFiles &inner = *fake;
    YieldingFiles files(std::move(fake),
                        [] { throw std::runtime_error("Interrupted"); });

    THEN("Each load fails with that error after reading its file") {
      REQUIRE_THROWS_WITH(files.loadBitmap("0137/0137_000.bmp"), "Interrupted");
      REQUIRE_THROWS_WITH(files.read("0385.json"), "Interrupted");
      REQUIRE(inner.loaded ==
              std::vector<std::string>{"0137/0137_000.bmp", "0385.json"});
    }
  }
}
