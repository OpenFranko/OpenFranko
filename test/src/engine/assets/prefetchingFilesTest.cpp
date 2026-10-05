#include "../../../../src/engine/assets/PrefetchingFiles.h"

#include "../../../../lib/converter/packedArchive/packedArchive.h"
#include "../../../../src/engine/assets/PackedFiles.h"
#include "FakeFiles.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace openfranko::src::engine::assets;
using openfranko::lib::converter::packedArchive::ArchiveWriter;
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

namespace {

constexpr int STEP_LIMIT = 1000;
constexpr auto BIG = "assets/big.bmp";
constexpr auto LATE = "assets/late.bmp";
constexpr auto SMALL = "assets/small.bmp";
constexpr auto MISSING = "assets/missing.bmp";

IndexedBitmap noise(int width, int height, uint32_t seed) {
  IndexedBitmap bitmap;
  bitmap.width = width;
  bitmap.height = height;
  bitmap.hotspotX = 4;
  bitmap.hotspotY = -2;
  bitmap.palette = {0x000, 0x0F0, static_cast<uint16_t>(seed & 0xFFF)};
  uint32_t state = seed;
  for (int at = 0; at < width * height; ++at) {
    state = state * 1103515245u + 12345u;
    bitmap.pixels.push_back(static_cast<uint8_t>(state >> 24));
  }
  return bitmap;
}

struct PackedPrefetch {
  PackedPrefetch() {
    ArchiveWriter writer;
    writer.addBitmap(BIG, big);
    writer.addBitmap(LATE, noise(320, 100, 2));
    writer.addBitmap(SMALL, picture(3));
    data = writer.finish();
    files = std::make_unique<PrefetchingFiles>(
        std::make_unique<PackedFiles>(data.data(), data.size()));
  }

  int stepsUntilIdle() {
    int steps = 0;
    while (files->step() && steps < STEP_LIMIT) {
      ++steps;
    }
    return steps;
  }

  int stepsToLoad(const std::string &path) {
    const auto load = files->beginBitmap(path);
    IndexedBitmap bitmap;
    int steps = 1;
    while (!load->step(bitmap) && steps < STEP_LIMIT) {
      ++steps;
    }
    return steps;
  }

  const IndexedBitmap big = noise(320, 100, 1);
  std::vector<uint8_t> data;
  std::unique_ptr<PrefetchingFiles> files;
};

} // namespace

SCENARIO("Bitmaps read ahead in steps are handed over whole") {
  GIVEN("A picture larger than a step, a missing one and a small one") {
    PackedPrefetch prefetch;
    prefetch.files->prefetch({BIG, MISSING, SMALL});

    WHEN("The prefetch steps until it is idle") {
      const int steps = prefetch.stepsUntilIdle();

      THEN("The large picture took several steps and the missing one one") {
        REQUIRE(steps > 3);
        REQUIRE(steps < STEP_LIMIT);
        REQUIRE_FALSE(prefetch.files->step());
      }

      THEN("A prefetched picture is handed over in a single step") {
        const auto load = prefetch.files->beginBitmap(BIG);
        IndexedBitmap bitmap;
        REQUIRE(load->step(bitmap));
        REQUIRE(bitmap.width == 320);
        REQUIRE(bitmap.hotspotX == 4);
        REQUIRE(bitmap.palette == prefetch.big.palette);
        REQUIRE(bitmap.pixels == prefetch.big.pixels);
      }

      THEN("It is handed over once, then read in steps again") {
        REQUIRE(prefetch.stepsToLoad(BIG) == 1);
        REQUIRE(prefetch.stepsToLoad(BIG) > 1);
      }

      THEN("A picture that was not read ahead is read in steps") {
        REQUIRE(prefetch.stepsToLoad(LATE) > 1);
      }

      THEN("The small picture is loaded from the copy read ahead") {
        REQUIRE(prefetch.files->loadBitmap(SMALL).pixels == picture(3).pixels);
      }

      THEN("The missing picture still fails when it is loaded") {
        REQUIRE_THROWS_WITH(prefetch.files->loadBitmap(MISSING),
                            "Failed to load bitmap: assets/missing.bmp");
      }
    }

    WHEN("The prefetch is dropped half way through the large picture") {
      REQUIRE(prefetch.files->step());
      prefetch.files->drop();

      THEN("Nothing is left to step and the picture is read again") {
        REQUIRE_FALSE(prefetch.files->step());
        REQUIRE(prefetch.stepsToLoad(BIG) > 1);
      }
    }
  }
}

SCENARIO("More bitmaps can be queued behind those read ahead") {
  GIVEN("One picture read ahead") {
    Prefetch prefetch;
    prefetch.files->prefetch({"a.bmp"});
    while (prefetch.files->step()) {
    }

    WHEN("Another is queued and read") {
      prefetch.files->prefetchMore({"b.bmp"});
      while (prefetch.files->step()) {
      }

      THEN("Both are kept and each was read once") {
        REQUIRE(prefetch.files->loadBitmap("a.bmp").width == 1);
        REQUIRE(prefetch.files->loadBitmap("b.bmp").width == 2);
        REQUIRE(prefetch.inner->loaded ==
                std::vector<std::string>{"a.bmp", "b.bmp"});
      }
    }

    WHEN("A new set replaces the queue") {
      prefetch.files->prefetch({"b.bmp"});
      while (prefetch.files->step()) {
      }

      THEN("The first picture is no longer kept") {
        REQUIRE(prefetch.files->loadBitmap("a.bmp").width == 1);
        REQUIRE(prefetch.inner->loaded ==
                std::vector<std::string>{"a.bmp", "b.bmp", "a.bmp"});
      }
    }
  }
}

SCENARIO("Lookups and plain reads go straight to the files") {
  GIVEN("A picture and a script behind prefetching files") {
    Prefetch prefetch;
    prefetch.inner->contents["data/0385.json"] = {'{', '}'};
    PrefetchingFiles &files = *prefetch.files;

    THEN("The files answer what exists and what a directory holds") {
      REQUIRE(files.exists("a.bmp"));
      REQUIRE(files.exists("data/0385.json"));
      REQUIRE_FALSE(files.exists("c.bmp"));
      REQUIRE(files.list("data") == std::vector<std::string>{"data/0385.json"});
      const auto walk = files.walk("data");
      std::string_view name;
      REQUIRE(walk->next(name));
      REQUIRE(name == "0385.json");
      REQUIRE_FALSE(walk->next(name));
    }

    THEN("A file is read at once or in a step from the files") {
      REQUIRE(files.read("data/0385.json") == std::vector<uint8_t>{'{', '}'});
      std::vector<uint8_t> data;
      REQUIRE(files.beginRead("data/0385.json")->step(data));
      REQUIRE(data == std::vector<uint8_t>{'{', '}'});
      REQUIRE(prefetch.inner->loaded ==
              std::vector<std::string>{"data/0385.json", "data/0385.json"});
    }

    THEN("A picture that was not read ahead is begun from the files") {
      IndexedBitmap bitmap;
      REQUIRE(files.beginBitmap("b.bmp")->step(bitmap));
      REQUIRE(bitmap.width == 2);
      REQUIRE(prefetch.inner->loaded == std::vector<std::string>{"b.bmp"});
    }
  }
}
