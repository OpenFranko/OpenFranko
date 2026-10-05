#include "../../../../../src/engine/states/shared/StepLoader.h"

#include "../../../../../lib/converter/packedArchive/packedArchive.h"
#include "../../../../../src/engine/assets/PackedFiles.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../assets/FakeFiles.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states::shared;
using namespace openfranko::test::src::systems::audio;
using openfranko::lib::converter::packedArchive::ArchiveWriter;
using openfranko::src::systems::graphics::IndexedBitmap;
using openfranko::test::src::engine::assets::FakeFiles;

namespace {

constexpr int STEP_LIMIT = 1000;
constexpr auto PICTURE = "assets/03B6.bmp";
constexpr auto TUNE = "assets/0259.s3m";

StepLoader::Step counted(std::vector<std::string> &log, std::string name,
                         int steps) {
  return [&log, name = std::move(name), steps, taken = 0]() mutable {
    log.push_back(name);
    return ++taken == steps;
  };
}

std::vector<uint8_t> tune() {
  std::vector<uint8_t> data(40000);
  uint32_t state = 3;
  for (uint8_t &byte : data) {
    state = state * 1103515245u + 12345u;
    byte = static_cast<uint8_t>(state >> 24);
  }
  return data;
}

IndexedBitmap picture() {
  IndexedBitmap bitmap;
  bitmap.width = 320;
  bitmap.height = 100;
  bitmap.hotspotX = 1;
  bitmap.palette = {0x000, 0x123};
  const std::vector<uint8_t> noise = tune();
  bitmap.pixels.assign(noise.begin(), noise.begin() + 320 * 100);
  return bitmap;
}

std::vector<uint8_t> archive() {
  ArchiveWriter writer;
  writer.addBitmap(PICTURE, picture());
  writer.addFile(TUNE, tune());
  return writer.finish();
}

class TuneSpeaker : public FakeSpeaker {
public:
  std::vector<uint8_t> handed;
  int begun = 0;

  std::unique_ptr<MusicLoad> beginMusic(const std::string &path,
                                        std::vector<uint8_t> data,
                                        int steps) override {
    handed = data;
    ++begun;
    return Speaker::beginMusic(path, std::move(data), steps);
  }
};

} // namespace

SCENARIO("Loading tasks run a step at a time, in the order added") {
  GIVEN("Three tasks taking one, three and two steps") {
    std::vector<std::string> log;
    StepLoader loader;
    const std::size_t first = loader.add(counted(log, "first", 1));
    const std::size_t second = loader.add(counted(log, "second", 3));
    const std::size_t third = loader.add(counted(log, "third", 2));

    THEN("Nothing runs before a step and no task is done") {
      REQUIRE(log.empty());
      REQUIRE_FALSE(loader.isDone(first));
    }

    WHEN("Two steps run") {
      loader.step(2);

      THEN("The first task is done and the second has begun") {
        REQUIRE(log == std::vector<std::string>{"first", "second"});
        REQUIRE(loader.isDone(first));
        REQUIRE_FALSE(loader.isDone(second));
      }
    }

    WHEN("More steps run than the tasks need") {
      loader.step(10);
      loader.step(5);

      THEN("Each task ran until it was done, and no more") {
        REQUIRE(log == std::vector<std::string>{"first", "second", "second",
                                                "second", "third", "third"});
        REQUIRE(loader.isDone(third));
      }
    }

    WHEN("The third task is wanted at once") {
      loader.finish(third);

      THEN("The tasks before it are finished first") {
        REQUIRE(log == std::vector<std::string>{"first", "second", "second",
                                                "second", "third", "third"});
        REQUIRE(loader.isDone(second));
        REQUIRE(loader.isDone(third));
      }
    }

    WHEN("A task that is done is wanted again") {
      loader.step(1);
      loader.finish(first);

      THEN("Nothing more runs") {
        REQUIRE(log == std::vector<std::string>{"first"});
        REQUIRE_FALSE(loader.isDone(second));
      }
    }
  }

  GIVEN("No tasks") {
    StepLoader loader;
    loader.step(3);
    loader.finish(0);

    THEN("Nothing is done") { REQUIRE_FALSE(loader.isDone(0)); }
  }
}

SCENARIO("A bitmap task reads its picture in steps") {
  GIVEN("A packed picture larger than a step") {
    const std::vector<uint8_t> data = archive();
    assets::PackedFiles files(data.data(), data.size());
    IndexedBitmap bitmap;
    StepLoader loader;
    const std::size_t task = loader.add(bitmapStep(files, PICTURE, bitmap));

    WHEN("It runs to the end") {
      int steps = 0;
      while (!loader.isDone(task) && steps < STEP_LIMIT) {
        loader.step(1);
        ++steps;
      }

      THEN("It took several steps and the picture is whole") {
        REQUIRE(steps > 1);
        REQUIRE(steps < STEP_LIMIT);
        REQUIRE(bitmap.width == 320);
        REQUIRE(bitmap.hotspotX == 1);
        REQUIRE(bitmap.palette == picture().palette);
        REQUIRE(bitmap.pixels == picture().pixels);
      }
    }
  }
}

SCENARIO("A music task reads the tune, then hands it to the speaker") {
  GIVEN("A packed tune larger than a step") {
    const std::vector<uint8_t> data = archive();
    assets::PackedFiles files(data.data(), data.size());
    TuneSpeaker speaker;
    StepLoader loader;
    const std::size_t task = loader.add(musicStep(files, speaker, TUNE));

    WHEN("It runs to the end") {
      loader.step(1);
      const int begunAfterFirstStep = speaker.begun;
      int steps = 1;
      while (!loader.isDone(task) && steps < STEP_LIMIT) {
        loader.step(1);
        ++steps;
      }

      THEN("The speaker gets the whole file once it has been read") {
        REQUIRE(begunAfterFirstStep == 0);
        REQUIRE(steps > 3);
        REQUIRE(steps < STEP_LIMIT);
        REQUIRE(speaker.begun == 1);
        REQUIRE(speaker.handed == tune());
        REQUIRE(speaker.music == TUNE);
      }
    }
  }

  GIVEN("Files that read the tune whole") {
    FakeFiles files;
    files.contents[TUNE] = {1, 2, 3};
    TuneSpeaker speaker;
    StepLoader loader;
    const std::size_t task = loader.add(musicStep(files, speaker, TUNE));

    WHEN("It runs to the end") {
      loader.finish(task);

      THEN("The speaker gets the file's bytes") {
        REQUIRE(speaker.handed == std::vector<uint8_t>{1, 2, 3});
        REQUIRE(speaker.music == TUNE);
      }
    }
  }

  GIVEN("Files without the tune") {
    FakeFiles files;
    TuneSpeaker speaker;
    StepLoader loader;
    const std::size_t task = loader.add(musicStep(files, speaker, TUNE));

    WHEN("A step runs") {
      loader.step(1);

      THEN("The speaker is asked to load the tune itself") {
        REQUIRE(loader.isDone(task));
        REQUIRE(speaker.begun == 0);
        REQUIRE(speaker.music == TUNE);
      }
    }
  }
}
