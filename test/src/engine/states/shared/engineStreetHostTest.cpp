#include "../../../../../src/engine/states/shared/EngineStreetHost.h"

#include "../../../../../lib/converter/packedArchive/packedArchive.h"
#include "../../../../../src/engine/MenuTempo.h"
#include "../../../../../src/engine/assets/PackedFiles.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../assets/FakeFiles.h"

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states::shared;
using namespace openfranko::test::src::systems::audio;
using openfranko::lib::converter::packedArchive::ArchiveWriter;
using openfranko::src::systems::graphics::IndexedBitmap;
using openfranko::test::src::engine::assets::FakeFiles;

namespace {

constexpr int SPRITE_SET = 0x12;
constexpr int SCENERY = 0x12C;
constexpr int BANK = 4;
constexpr int BASE = 10;
constexpr int STEP_LIMIT = 200;
constexpr int BIG_SET = 0x15;
constexpr int BIG_FRAMES = 60;
constexpr int BIG_SAMPLES = 3;
constexpr int BUDGET = 20;
constexpr int MUSIC = 0x259;
constexpr int FRAME_BY_FRAME = street::scenes::StreetHost::FRAME_BY_FRAME;

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
  for (int index = 0; index < BIG_FRAMES; ++index) {
    char name[32];
    std::snprintf(name, sizeof(name), "assets/0015/0015_%03d.bmp", index);
    writer.addBitmap(name, frame(index + 1));
  }
  writer.addFile("assets/0259.s3m", std::vector<uint8_t>(30000, 7));
  for (int sample = 1; sample <= BIG_SAMPLES; ++sample) {
    writer.addFile("assets/0015/0015_sam" + std::to_string(sample) +
                       "_5000Hz.wav",
                   {static_cast<uint8_t>(sample)});
  }
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
      const auto load =
          fixture.host.beginSpriteSet(SPRITE_SET, BANK, BASE, FRAME_BY_FRAME);
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
        const auto load =
            fixture.host.beginSpriteSet(resource, BANK, BASE, FRAME_BY_FRAME);
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
      const auto load =
          fixture.host.beginSpriteSet(SPRITE_SET, 0, BASE, FRAME_BY_FRAME);
      int steps = 1;
      while (!load->step(images) && steps < STEP_LIMIT) {
        ++steps;
      }

      THEN("Only the frames are loaded") {
        REQUIRE(images.find(BASE + 7)->width == 8);
        REQUIRE(fixture.speaker.samples.empty());
      }
    }

    WHEN("A big set is loaded within a budget of steps") {
      street::core::ImageBank images;
      const auto load =
          fixture.host.beginSpriteSet(BIG_SET, BANK, BASE, BUDGET);
      int steps = 1;
      while (!load->step(images) && steps < STEP_LIMIT) {
        ++steps;
      }

      THEN("It fits the budget and loads every frame and sample") {
        REQUIRE(steps <= BUDGET);
        for (int index = 0; index < BIG_FRAMES; ++index) {
          CAPTURE(index);
          REQUIRE(images.find(BASE + index)->width == index + 1);
        }
        REQUIRE(fixture.speaker.samples.size() == BIG_SAMPLES);
      }
    }

    WHEN("The big set is loaded frame by frame") {
      street::core::ImageBank images;
      const auto load =
          fixture.host.beginSpriteSet(BIG_SET, BANK, BASE, FRAME_BY_FRAME);
      int steps = 1;
      while (!load->step(images) && steps < STEP_LIMIT) {
        ++steps;
      }

      THEN("Each frame takes a step of its own") {
        REQUIRE(steps > BIG_FRAMES + BIG_SAMPLES);
        REQUIRE(images.find(BASE + BIG_FRAMES - 1)->width == BIG_FRAMES);
      }
    }

    WHEN("A tune is loaded in steps") {
      const auto load = fixture.host.beginMusic(MUSIC, BUDGET);
      int steps = 1;
      while (!load->step() && steps < STEP_LIMIT) {
        ++steps;
      }

      THEN("The file is read first and the speaker gets the tune") {
        REQUIRE(steps > 2);
        REQUIRE(steps <= BUDGET);
        REQUIRE(fixture.speaker.music == "assets/0259.s3m");
      }
    }

    WHEN("A missing tune is loaded in steps") {
      const auto load = fixture.host.beginMusic(MUSIC + 1, BUDGET);
      int steps = 1;
      while (!load->step() && steps < STEP_LIMIT) {
        ++steps;
      }

      THEN("The speaker is asked for it like before") {
        REQUIRE(steps == 1);
        REQUIRE(fixture.speaker.music == "assets/025A.s3m");
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
        const auto load =
            fixture.host.beginSpriteSet(0x13, BANK, BASE, FRAME_BY_FRAME);
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

namespace {

constexpr int PICTURE = 0x388;
constexpr int MISSING_PICTURE = 0x389;
constexpr int LEVEL_SCRIPT = 0x385;
constexpr int MISSING_LEVEL_SCRIPT = 0x386;
constexpr int WIDE_SCENERY = 0x12D;
constexpr int WIDE_COLUMNS = 20;
constexpr int PANEL_PART = 2;
constexpr int OTHER_SET = 0x14;
constexpr uint32_t SEED = 9;

constexpr auto LEVEL_JSON = R"({"lengthInColumns": 40, "waves": [
  {"triggerColumn": 3, "slots": [null, {"spriteSetId": 18, "kind": 2,
    "spawnX": 300, "spawnY": 150, "energy": 20, "aggression": 4}, null]},
  {"triggerColumn": 9, "slots": [null, null, null]},
  {"triggerColumn": 12, "slots": [null, null, null]}]})";

constexpr auto CREDITS_JSON = R"({"pages": [
  {"beat": 4, "lines": [{"text": "FRANKO", "y": 20}, {"text": "ALEX", "y": 40}]},
  {"beat": 8, "lines": [{"text": "KONIEC", "y": 60}]}]})";

std::vector<uint8_t> text(const std::string &json) {
  return std::vector<uint8_t>(json.begin(), json.end());
}

IndexedBitmap picture(int width, int height, int seed) {
  IndexedBitmap bitmap;
  bitmap.width = width;
  bitmap.height = height;
  bitmap.hotspotX = seed;
  bitmap.hotspotY = -seed;
  bitmap.palette = {0x000, static_cast<uint16_t>(0x100 * seed), 0xFFF};
  for (int at = 0; at < width * height; ++at) {
    bitmap.pixels.push_back(static_cast<uint8_t>((at * seed + at / 7) % 3));
  }
  return bitmap;
}

std::vector<uint8_t> screensArchive() {
  ArchiveWriter writer;
  writer.addBitmap("assets/0388.bmp", picture(200, 100, 3));
  writer.addBitmap("assets/0384/0384_2.bmp", picture(16, 4, 5));
  writer.addFile("assets/0385.json", text(LEVEL_JSON));
  writer.addFile("assets/credits.json", text(CREDITS_JSON));
  for (int column = 0; column < WIDE_COLUMNS; ++column) {
    char name[32];
    std::snprintf(name, sizeof(name), "assets/012D/012D_%03d.bmp", column);
    writer.addBitmap(name, frame(column + 1));
  }
  writer.addBitmap("assets/0012/0012_000.bmp", frame(1));
  writer.addFile("assets/0012/0012_sam1_5000Hz.wav", {1});
  writer.addFile("assets/0012/0012_sam2_5000Hz.wav", {2});
  writer.addBitmap("assets/0014/0014_000.bmp", frame(2));
  writer.addFile("assets/0014/0014_sam3_5000Hz.wav", {3});
  return writer.finish();
}

class TunedSpeaker : public FakeSpeaker {
public:
  std::vector<int> frequencies;

  void playSampleAt(const std::string &name, int voiceMask,
                    int frequency) override {
    FakeSpeaker::playSampleAt(name, voiceMask, frequency);
    frequencies.push_back(frequency);
  }
};

struct Screens {
  std::vector<uint8_t> data = screensArchive();
  assets::PackedFiles files{data.data(), data.size()};
  TunedSpeaker speaker;
  MersenneTwister random{SEED};
  int yields = 0;
  std::optional<EngineStreetHost> host{std::in_place, speaker,
                                       files,         GameVersion::V10,
                                       random,        [this] { ++yields; }};
};

template <typename Load, typename... Targets>
int stepToEnd(Load &load, Targets &&...targets) {
  int steps = 1;
  while (!load.step(targets...) && steps < STEP_LIMIT) {
    ++steps;
  }
  return steps;
}

void requireSameScript(const street::core::LevelScript &script,
                       const street::core::LevelScript &expected) {
  REQUIRE(script.length == expected.length);
  REQUIRE(script.waves.size() == expected.waves.size());
  for (std::size_t wave = 0; wave < expected.waves.size(); ++wave) {
    CAPTURE(wave);
    REQUIRE(script.waves[wave].trigger == expected.waves[wave].trigger);
    for (std::size_t slot = 0; slot < expected.waves[wave].slots.size();
         ++slot) {
      const street::core::EnemySlot &found = script.waves[wave].slots[slot];
      const street::core::EnemySlot &wanted = expected.waves[wave].slots[slot];
      REQUIRE(found.spriteSet == wanted.spriteSet);
      REQUIRE(found.type == wanted.type);
      REQUIRE(found.x == wanted.x);
      REQUIRE(found.y == wanted.y);
      REQUIRE(found.energy == wanted.energy);
      REQUIRE(found.aggression == wanted.aggression);
    }
  }
}

std::vector<std::string>
creditTexts(const street::core::EndingCredits &credits) {
  std::vector<std::string> texts;
  for (const street::core::CreditPage &page : credits.pages) {
    texts.push_back("beat " + std::to_string(page.beat));
    for (const street::core::CreditLine &line : page.lines) {
      texts.push_back(line.text + " at " + std::to_string(line.y));
    }
  }
  return texts;
}

const std::vector<std::string> CREDIT_TEXTS = {
    "beat 4", "FRANKO at 20", "ALEX at 40", "beat 8", "KONIEC at 60"};

} // namespace

SCENARIO("The street host reads pictures, their palettes and panel parts") {
  GIVEN("A packed picture and a part of the panel") {
    Screens screens;
    const IndexedBitmap expected = picture(200, 100, 3);

    THEN("The picture keeps its size, hotspot and pixels") {
      const street::core::Picture loaded = screens.host->loadPicture(PICTURE);
      REQUIRE(loaded.width == 200);
      REQUIRE(loaded.height == 100);
      REQUIRE(loaded.hotX == 3);
      REQUIRE(loaded.hotY == -3);
      REQUIRE(loaded.pixels == expected.pixels);
    }

    THEN("Its palette can be read on its own") {
      REQUIRE(screens.host->loadPalette(PICTURE) == expected.palette);
    }

    WHEN("The picture is loaded in steps with its palette") {
      street::core::Picture stepped;
      effects::color::AmigaPalette palette;
      const auto load = screens.host->beginPicture(PICTURE);
      const int steps = stepToEnd(*load, stepped, &palette);

      THEN("It takes several steps and matches loading it at once") {
        REQUIRE(steps > 1);
        REQUIRE(steps < STEP_LIMIT);
        REQUIRE(stepped.pixels == expected.pixels);
        REQUIRE(stepped.hotX == 3);
        REQUIRE(palette == expected.palette);
      }
    }

    WHEN("The picture is loaded in steps without a palette") {
      street::core::Picture stepped;
      const auto load = screens.host->beginPicture(PICTURE);
      const int steps = stepToEnd(*load, stepped, nullptr);

      THEN("Only the picture is filled in") {
        REQUIRE(steps < STEP_LIMIT);
        REQUIRE(stepped.width == 200);
        REQUIRE(stepped.pixels == expected.pixels);
      }
    }

    THEN("A panel part is read from the panel's folder") {
      const street::core::Picture part =
          screens.host->loadPanelPicture(PANEL_PART);
      REQUIRE(part.width == 16);
      REQUIRE(part.hotX == 5);
      REQUIRE(part.pixels == picture(16, 4, 5).pixels);
    }

    THEN("A picture that is not there fails to load") {
      REQUIRE_THROWS(screens.host->loadPicture(MISSING_PICTURE));
    }
  }
}

SCENARIO("The street host reads level scripts and the ending credits") {
  GIVEN("A level script of three waves and two pages of credits") {
    Screens screens;

    WHEN("The script is loaded at once") {
      const street::core::LevelScript script =
          screens.host->loadLevelScript(LEVEL_SCRIPT);

      THEN("Every wave and slot is read and the host yields once") {
        REQUIRE(script.length == 40);
        REQUIRE(script.waves.size() == 3);
        REQUIRE(script.waves[2].trigger == 12);
        const street::core::EnemySlot &enemy = script.waves[0].slots[1];
        REQUIRE(enemy.spriteSet == 18);
        REQUIRE(enemy.type == 2);
        REQUIRE(enemy.x == 300);
        REQUIRE(enemy.y == 150);
        REQUIRE(enemy.energy == 20);
        REQUIRE(enemy.aggression == 4);
        REQUIRE(script.waves[0].slots[0].spriteSet ==
                street::core::EnemySlot::EMPTY);
        REQUIRE(screens.yields == 1);
      }
    }

    WHEN("The script is loaded in steps") {
      street::core::LevelScript script;
      const auto load = screens.host->beginLevelScript(LEVEL_SCRIPT);
      const int steps = stepToEnd(*load, script);

      THEN("It takes a step per wave at least and matches loading it at once") {
        REQUIRE(steps > 3);
        REQUIRE(steps < STEP_LIMIT);
        requireSameScript(script, screens.host->loadLevelScript(LEVEL_SCRIPT));
      }
    }

    WHEN("The credits are loaded at once") {
      const street::core::EndingCredits credits =
          screens.host->loadEndingCredits();

      THEN("Each page keeps its beat and its lines") {
        REQUIRE(creditTexts(credits) == CREDIT_TEXTS);
      }
    }

    WHEN("The credits are loaded in steps") {
      street::core::EndingCredits credits;
      const auto load = screens.host->beginEndingCredits();
      const int steps = stepToEnd(*load, credits);

      THEN("They match loading them at once") {
        REQUIRE(steps > 1);
        REQUIRE(steps < STEP_LIMIT);
        REQUIRE(creditTexts(credits) == CREDIT_TEXTS);
        REQUIRE(screens.yields == 0);
      }
    }
  }

  GIVEN("Files without the level script or the credits") {
    FakeFiles files;
    FakeSpeaker speaker;
    MersenneTwister random{SEED};
    EngineStreetHost host{speaker, files, GameVersion::V10, random, [] {}};

    THEN("Loading them fails with the path that is missing") {
      constexpr auto MISSING_SCRIPT =
          "Failed to open level script: assets/0386.json";
      constexpr auto MISSING_CREDITS =
          "Failed to open ending credits: assets/credits.json";
      REQUIRE_THROWS_WITH(host.loadLevelScript(MISSING_LEVEL_SCRIPT),
                          MISSING_SCRIPT);
      street::core::LevelScript script;
      REQUIRE_THROWS_WITH(
          host.beginLevelScript(MISSING_LEVEL_SCRIPT)->step(script),
          MISSING_SCRIPT);
      REQUIRE_THROWS_WITH(host.loadEndingCredits(), MISSING_CREDITS);
      street::core::EndingCredits credits;
      REQUIRE_THROWS_WITH(host.beginEndingCredits()->step(credits),
                          MISSING_CREDITS);
      REQUIRE(files.loaded.empty());
    }
  }
}

SCENARIO("The street host passes music and samples on to the speaker") {
  GIVEN("A host on a speaker") {
    Screens screens;
    EngineStreetHost &host = *screens.host;

    WHEN("A tune is loaded, played, turned down, sped up and stopped") {
      host.loadMusic(MUSIC);
      host.playMusic();
      host.setMusicVolume(40);
      host.setMusicTempo(CONVERTED_MENU_TEMPO);
      host.setMusicTempo(2 * CONVERTED_MENU_TEMPO);
      host.stopMusic();

      THEN("The speaker gets the tune's file and each call in turn") {
        REQUIRE(screens.speaker.music == "assets/0259.s3m");
        REQUIRE(host.isMusicLoaded(MUSIC));
        REQUIRE_FALSE(host.isMusicLoaded(MUSIC + 1));
        REQUIRE(screens.speaker.musicStarts == 1);
        REQUIRE(screens.speaker.volumes == std::vector<int>{40});
        REQUIRE(screens.speaker.tempoScales == std::vector<double>{1.0, 2.0});
        REQUIRE(screens.speaker.musicStops == 1);
      }
    }

    WHEN("Samples are played from several banks") {
      host.playSample(BANK, 1, 0x3);
      host.playSampleAt(2, 7, 0x1, 8000);
      host.playSample(BANK, 1, 0x2);
      host.playSample(-1, 2, 0x4);
      host.playSample(3, -4, 0x8);

      THEN("Each is named by its bank and number, at the frequency asked") {
        REQUIRE(screens.speaker.plays ==
                std::vector<FakeSpeaker::Play>{{"streetBank4Sample1", 0x3},
                                               {"streetBank2Sample7", 0x1},
                                               {"streetBank4Sample1", 0x2},
                                               {"streetBank-1Sample2", 0x4},
                                               {"streetBank3Sample-4", 0x8}});
        REQUIRE(screens.speaker.frequencies == std::vector<int>{8000});
        REQUIRE(EngineStreetHost::sampleName(12, 30) == "streetBank12Sample30");
      }
    }

    WHEN("A bank's samples loop and the host goes") {
      host.loadSpriteSet(SPRITE_SET, BANK);
      host.setSampleLooping(true);
      REQUIRE(screens.speaker.sampleLooping);
      REQUIRE(screens.speaker.samples.size() == 2);
      screens.host.reset();

      THEN("Looping is switched off and the bank's samples are cleared") {
        REQUIRE_FALSE(screens.speaker.sampleLooping);
        REQUIRE(screens.speaker.samples.empty());
      }
    }

    WHEN("Another set is loaded at once into the same bank") {
      host.loadSpriteSet(SPRITE_SET, BANK);
      host.loadSpriteSet(OTHER_SET, BANK);

      THEN("Only the samples of the second set remain in the bank") {
        REQUIRE(
            screens.speaker.samples ==
            std::map<std::string, std::string>{
                {"streetBank4Sample3", "assets/0014/0014_sam3_5000Hz.wav"}});
      }
    }
  }
}

SCENARIO("The street host draws numbers below a limit and yields on request") {
  GIVEN("A host on a seeded generator") {
    Screens screens;
    MersenneTwister reference{SEED};

    THEN("Limits of zero or less give zero without a draw") {
      for (const int limit : {6, 0, 1, -3, 100, 0, 6}) {
        CAPTURE(limit);
        const int expected =
            limit > 0
                ? static_cast<int>(reference.upTo(static_cast<uint32_t>(limit)))
                : 0;
        REQUIRE(screens.host->random(limit) == expected);
      }
    }

    THEN("Each yield is passed to the owner") {
      screens.host->yield();
      screens.host->yield();
      REQUIRE(screens.yields == 2);
    }
  }
}

SCENARIO("Scenery with many columns is listed over several steps") {
  GIVEN("A chunk of twenty columns") {
    Screens screens;

    WHEN("It is loaded in steps") {
      std::vector<street::core::Picture> columns;
      const auto load = screens.host->beginScenery(WIDE_SCENERY);
      const int steps = stepToEnd(*load, columns);

      THEN("Every column arrives, as when it is loaded at once") {
        std::vector<int> expected;
        for (int column = 1; column <= WIDE_COLUMNS; ++column) {
          expected.push_back(column);
        }
        REQUIRE(steps > WIDE_COLUMNS / 2);
        REQUIRE(steps < STEP_LIMIT);
        REQUIRE(widths(columns) == expected);
        REQUIRE(widths(screens.host->loadScenery(WIDE_SCENERY)) == expected);
      }
    }
  }
}

SCENARIO("The host's files are named for the game version and directory") {
  GIVEN("A 1.2 host in the default directory") {
    FakeFiles files;
    files.bitmaps["assets/s18/s18_000.bmp"] = frame(5);
    FakeSpeaker speaker;
    MersenneTwister random{SEED};
    const auto host = std::make_unique<EngineStreetHost>(
        speaker, files, GameVersion::V12, random, [] {});

    THEN("It reports 1.2 and reads the 1.2 files") {
      REQUIRE(host->version() == GameVersion::V12);
      host->loadPicture(PICTURE);
      host->loadPanelPicture(PANEL_PART);
      REQUIRE(widths(host->loadSpriteSet(SPRITE_SET, 0)) ==
              std::vector<int>{5});
      host->loadMusic(MUSIC);
      REQUIRE(files.loaded ==
              std::vector<std::string>{"assets/p4.bmp", "assets/p0/p0_2.bmp",
                                       "assets/s18/s18_000.bmp"});
      REQUIRE(speaker.music == "assets/m1.s3m");
      REQUIRE(host->isMusicLoaded(MUSIC));
    }
  }

  GIVEN("A 1.0 host in another directory") {
    FakeFiles files;
    files.contents["data/credits.json"] = text(CREDITS_JSON);
    FakeSpeaker speaker;
    MersenneTwister random{SEED};
    EngineStreetHost host{speaker, files, GameVersion::V10,
                          random,  [] {}, "data"};

    THEN("It reports 1.0 and reads from that directory") {
      REQUIRE(host.version() == GameVersion::V10);
      host.loadPicture(PICTURE);
      host.loadMusic(MUSIC);
      REQUIRE(creditTexts(host.loadEndingCredits()) == CREDIT_TEXTS);
      REQUIRE(files.loaded ==
              std::vector<std::string>{"data/0388.bmp", "data/credits.json"});
      REQUIRE(speaker.music == "data/0259.s3m");
    }
  }
}
