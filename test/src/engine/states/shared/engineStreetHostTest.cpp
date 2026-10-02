#include "../../../../../src/engine/states/shared/EngineStreetHost.h"

#include "../../../../../lib/converter/packedArchive/packedArchive.h"
#include "../../../../../src/engine/assets/PackedFiles.h"
#include "../../../systems/audio/FakeSpeaker.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states::shared;
using namespace openfranko::test::src::systems::audio;
using openfranko::lib::converter::packedArchive::ArchiveWriter;
using openfranko::src::systems::graphics::IndexedBitmap;

namespace {

constexpr int SPRITE_SET = 0x12;
constexpr int SCENERY = 0x12C;
constexpr int BANK = 4;
constexpr int BASE = 10;
constexpr int STEP_LIMIT = 200;

IndexedBitmap frame(int width) {
  IndexedBitmap bitmap;
  bitmap.width = width;
  bitmap.height = 3;
  bitmap.hotspotX = width / 2;
  bitmap.hotspotY = 1;
  bitmap.palette = {0x000, 0xFFF};
  bitmap.pixels.assign(static_cast<std::size_t>(width * bitmap.height),
                       static_cast<uint8_t>(width));
  return bitmap;
}

std::vector<uint8_t> archive() {
  ArchiveWriter writer;
  writer.addBitmap("assets/0012/0012_000.bmp", frame(1));
  writer.addBitmap("assets/0012/0012_001.bmp", frame(2));
  writer.addBitmap("assets/0012/0012_003.bmp", frame(4));
  writer.addBitmap("assets/0012/0012_005.bmp.old", frame(6));
  writer.addBitmap("assets/0012/0012_7.bmp", frame(8));
  writer.addFile("assets/0012/0012_sam1_5000Hz.wav", {1, 2, 3});
  writer.addFile("assets/0012/0012_sam2_6000Hz.wav", {4, 5, 6});
  writer.addFile("assets/0012/notes.txt", {7});
  writer.addBitmap("assets/012C/012C_000.bmp", frame(3));
  writer.addBitmap("assets/012C/012C_002.bmp", frame(5));
  writer.addFile("assets/0013/0013_sam1_5000Hz.wav", {8});
  writer.addBitmap("assets/0014/0014_000.bmp", frame(9));
  writer.addFile("assets/0014/0014_sam1_7000Hz.wav", {9});
  return writer.finish();
}

struct Fixture {
  std::vector<uint8_t> data = archive();
  assets::PackedFiles files{data.data(), data.size()};
  FakeSpeaker speaker;
  MersenneTwister random{1};
  EngineStreetHost host{speaker, files, GameVersion::V10, random, [] {}};
};

std::vector<int> widths(const std::vector<street::core::Picture> &frames) {
  std::vector<int> found;
  for (const street::core::Picture &picture : frames) {
    found.push_back(picture.width);
  }
  return found;
}

const std::map<std::string, std::string> SAMPLES = {
    {"streetBank4Sample1", "assets/0012/0012_sam1_5000Hz.wav"},
    {"streetBank4Sample2", "assets/0012/0012_sam2_6000Hz.wav"}};

} // namespace

SCENARIO("The street host loads numbered frames and samples from a set") {
  GIVEN("A packed set with a missing frame, odd names and samples") {
    Fixture fixture;

    WHEN("The whole set is loaded") {
      const auto frames = fixture.host.loadSpriteSet(SPRITE_SET, BANK);

      THEN("Every numbered frame lands at its number and gaps stay empty") {
        REQUIRE(widths(frames) == std::vector<int>{1, 2, 0, 4, 0, 6, 0, 8});
        REQUIRE(frames[3].pixels == std::vector<uint8_t>(12, 4));
        REQUIRE(frames[3].hotX == 2);
      }

      THEN("The samples are loaded into the bank by their own paths") {
        REQUIRE(fixture.speaker.samples == SAMPLES);
      }
    }

    WHEN("The set is loaded in steps") {
      street::core::ImageBank images;
      const auto load = fixture.host.beginSpriteSet(SPRITE_SET, BANK, BASE);
      int steps = 1;
      while (!load->step(images) && steps < STEP_LIMIT) {
        ++steps;
      }

      THEN("It takes several steps and ends with the same images") {
        REQUIRE(steps > 1);
        REQUIRE(steps < STEP_LIMIT);
        const std::vector<int> expected{1, 2, 0, 4, 0, 6, 0, 8};
        for (int index = 0; index < static_cast<int>(expected.size());
             ++index) {
          CAPTURE(index);
          const street::core::Picture *picture = images.find(BASE + index);
          REQUIRE((picture ? picture->width : 0) == expected[index]);
        }
        REQUIRE(fixture.speaker.samples == SAMPLES);
      }
    }

    WHEN("Another set is loaded in steps into the same bank") {
      street::core::ImageBank images;
      for (const int resource : {SPRITE_SET, 0x14}) {
        const auto load = fixture.host.beginSpriteSet(resource, BANK, BASE);
        for (int step = 0; step < STEP_LIMIT && !load->step(images); ++step) {
        }
      }

      THEN("Only the samples of the second set remain in the bank") {
        REQUIRE(
            fixture.speaker.samples ==
            std::map<std::string, std::string>{
                {"streetBank4Sample1", "assets/0014/0014_sam1_7000Hz.wav"}});
        REQUIRE(images.find(BASE)->width == 9);
      }
    }

    WHEN("The set is loaded in steps without a sample bank") {
      street::core::ImageBank images;
      const auto load = fixture.host.beginSpriteSet(SPRITE_SET, 0, BASE);
      int steps = 1;
      while (!load->step(images) && steps < STEP_LIMIT) {
        ++steps;
      }

      THEN("Only the frames are loaded") {
        REQUIRE(images.find(BASE + 7)->width == 8);
        REQUIRE(fixture.speaker.samples.empty());
      }
    }

    WHEN("A scenery chunk is loaded in steps") {
      std::vector<street::core::Picture> columns;
      const auto load = fixture.host.beginScenery(SCENERY);
      int steps = 1;
      while (!load->step(columns) && steps < STEP_LIMIT) {
        ++steps;
      }

      THEN("It matches loading the chunk at once") {
        REQUIRE(widths(columns) == std::vector<int>{3, 0, 5});
        REQUIRE(widths(columns) == widths(fixture.host.loadScenery(SCENERY)));
      }
    }

    WHEN("A set has samples but no frames") {
      THEN("Loading it fails like before") {
        REQUIRE_THROWS_WITH(fixture.host.loadSpriteSet(0x13, BANK),
                            "No frames found in assets/0013");
        street::core::ImageBank images;
        const auto load = fixture.host.beginSpriteSet(0x13, BANK, BASE);
        REQUIRE_THROWS_WITH(
            [&] {
              for (int step = 0; step < STEP_LIMIT; ++step) {
                if (load->step(images)) {
                  return;
                }
              }
            }(),
            "No frames found in assets/0013");
      }
    }
  }
}
