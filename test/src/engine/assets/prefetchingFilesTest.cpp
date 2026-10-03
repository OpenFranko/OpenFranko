#include "../../../../src/engine/assets/PrefetchingFiles.h"

#include "FakeFiles.h"

#include <catch2/catch_all.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace openfranko::src::engine::assets;
using openfranko::src::systems::graphics::IndexedBitmap;
using openfranko::test::src::engine::assets::FakeFiles;

namespace {

IndexedBitmap picture(int width) {
  IndexedBitmap bitmap;
  bitmap.width = width;
  bitmap.height = 1;
  bitmap.pixels.assign(static_cast<std::size_t>(width), 1);
  return bitmap;
}

struct Prefetch {
  Prefetch() {
    auto fake = std::make_unique<FakeFiles>();
    fake->bitmaps["a.bmp"] = picture(1);
    fake->bitmaps["b.bmp"] = picture(2);
    inner = fake.get();
    files = std::make_unique<PrefetchingFiles>(std::move(fake));
  }

  FakeFiles *inner = nullptr;
  std::unique_ptr<PrefetchingFiles> files;
};

} // namespace

SCENARIO("Bitmaps are read ahead and handed over once") {
  GIVEN("Two pictures to read ahead and one that is missing") {
    Prefetch prefetch;
    prefetch.files->prefetch({"a.bmp", "missing.bmp", "b.bmp"});

    WHEN("Enough steps have run") {
      for (int step = 0; step < 5; ++step) {
        prefetch.files->step();
      }

      THEN("Each picture was read once, in order") {
        REQUIRE(prefetch.inner->loaded ==
                std::vector<std::string>{"a.bmp", "missing.bmp", "b.bmp"});
      }

      THEN("Loading them takes the copies read ahead") {
        REQUIRE(prefetch.files->loadBitmap("b.bmp").width == 2);
        REQUIRE(prefetch.files->loadBitmap("a.bmp").width == 1);
        REQUIRE(prefetch.inner->loaded.size() == 3);
      }

      THEN("A second load reads the file again") {
        prefetch.files->loadBitmap("a.bmp");
        REQUIRE(prefetch.files->loadBitmap("a.bmp").width == 1);
        REQUIRE(prefetch.inner->loaded.size() == 4);
      }
    }

    WHEN("The pictures are dropped before use") {
      prefetch.files->step();
      prefetch.files->drop();

      THEN("Loading reads the file and no more steps read ahead") {
        prefetch.files->step();
        REQUIRE(prefetch.files->loadBitmap("a.bmp").width == 1);
        REQUIRE(prefetch.inner->loaded ==
                std::vector<std::string>{"a.bmp", "a.bmp"});
      }
    }
  }
}
