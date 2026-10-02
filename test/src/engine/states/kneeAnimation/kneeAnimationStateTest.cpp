#include "../../../../../src/engine/states/kneeAnimation/KneeAnimationState.h"

#include "../../../../../src/engine/assets/Assets.h"
#include "../../../../../src/systems/audio/Mixer.h"
#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::kneeAnimation;
using namespace openfranko::src::systems::audio;
using namespace openfranko::src::systems::graphics;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::systems::audio;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr auto SAMPLE = "knee";
constexpr int OPEN_FRAMES = 2;
constexpr int FIRST_PICTURE_FRAMES = 19;
constexpr int SAMPLE_FRAME = OPEN_FRAMES + 40;
constexpr int MUSIC_FRAME = OPEN_FRAMES + 100;
constexpr int GONE_FRAME = OPEN_FRAMES + 184;
constexpr int FRAMES = OPEN_FRAMES + 190;
constexpr std::array<uint16_t, 4> PART_COLORS = {0xF00, 0x0F0, 0x00F, 0xFFF};

FakeFiles kneeFiles(const std::string &images, const std::string &sample) {
  FakeFiles files;
  for (int part = 0; part < 4; ++part) {
    IndexedBitmap bitmap;
    bitmap.width = 320;
    bitmap.height = 256;
    bitmap.pixels.assign(static_cast<std::size_t>(320) * 256, 1);
    bitmap.palette = {0x000, PART_COLORS[static_cast<std::size_t>(part)]};
    files.bitmaps[assets::partPath(images, part)] = bitmap;
  }
  files.contents[sample] = {};
  return files;
}

void latchFire(ControllerSystem &controller) {
  KeyEvent space;
  space.key = Key::Space;
  space.pressed = true;
  controller.receiveKey(space);
  controller.update();
}

struct Knee {
  explicit Knee(GameVersion version)
      : files(version == GameVersion::V12
                  ? kneeFiles("p51", "assets/s50/s50_sam3_8363Hz.wav")
                  : kneeFiles("03B7", "assets/0263/0263_sam2_8363Hz.wav")) {
    latchFire(controller);
    state.emplace(monitor, speaker, controller, files, version);
  }

  FakeMonitor monitor;
  FakeSpeaker speaker;
  ControllerSystem controller;
  FakeFiles files;
  std::optional<KneeAnimationState> state;
};

} // namespace

SCENARIO("The knee's four pictures, sample and the menu tune are loaded") {
  GIVEN("Version 1.0") {
    Knee knee(GameVersion::V10);

    THEN("Nothing is read before the first frame") {
      REQUIRE(knee.files.loaded.empty());
    }

    WHEN("The grey screen has shown until the first picture is due") {
      run(*knee.state, OPEN_FRAMES + FIRST_PICTURE_FRAMES);

      THEN("The parts of 03B7 are read in order") {
        REQUIRE(std::vector<std::string>(knee.files.loaded.begin(),
                                         knee.files.loaded.begin() + 4) ==
                std::vector<std::string>{
                    "assets/03B7/03B7.bmp", "assets/03B7/03B7_1.bmp",
                    "assets/03B7/03B7_2.bmp", "assets/03B7/03B7_3.bmp"});
      }

      THEN("The knee comes from bank 0263 and the tune is 0261") {
        REQUIRE(knee.speaker.samples ==
                std::map<std::string, std::string>{
                    {SAMPLE, "assets/0263/0263_sam2_8363Hz.wav"}});
        REQUIRE(knee.speaker.music == "assets/0261.s3m");
      }
    }

    THEN("A fire pressed before is forgotten") {
      REQUIRE_FALSE(knee.controller.isFireLatched());
    }

    WHEN("The state is left") {
      knee.state.reset();

      THEN("The knee is cleared") { REQUIRE(knee.speaker.samples.empty()); }
    }
  }

  GIVEN("Version 1.2") {
    Knee knee(GameVersion::V12);

    THEN("The pictures are p51's, the knee is s50's third and the tune m9") {
      run(*knee.state, OPEN_FRAMES + FIRST_PICTURE_FRAMES);
      REQUIRE(knee.files.loaded.front() == "assets/p51/p51.bmp");
      REQUIRE(knee.speaker.samples ==
              std::map<std::string, std::string>{
                  {SAMPLE, "assets/s50/s50_sam3_8363Hz.wav"}});
      REQUIRE(knee.speaker.music == "assets/m9.s3m");
    }

    THEN("A fire pressed before still counts") {
      REQUIRE(knee.controller.isFireLatched());
    }
  }
}

SCENARIO("The knee is unpacked a picture every 20 frames on grey") {
  GIVEN("Version 1.0") {
    Knee knee(GameVersion::V10);

    THEN("The screens open black, then grey until the first picture") {
      run(*knee.state, OPEN_FRAMES);
      REQUIRE(knee.monitor.pixel(10, 10) == toArgb(0x000));
      run(*knee.state, 1);
      REQUIRE(knee.monitor.pixel(10, 10) == toArgb(0x555));
      run(*knee.state, 19);
      REQUIRE(knee.monitor.pixel(10, 10) == toArgb(0x555));
    }

    THEN("Each picture replaces the one before") {
      for (std::size_t part = 0; part < PART_COLORS.size(); ++part) {
        run(*knee.state, part == 0 ? OPEN_FRAMES + 21 : 20);
        REQUIRE(knee.monitor.pixel(10, 10) == toArgb(PART_COLORS[part]));
      }
    }

    THEN("The screen goes black again before the state ends") {
      run(*knee.state, GONE_FRAME);
      REQUIRE(knee.monitor.pixel(10, 10) == toArgb(PART_COLORS[3]));
      run(*knee.state, 1);
      REQUIRE(knee.monitor.pixel(10, 10) == toArgb(0x000));
    }
  }
}

SCENARIO("The knee is heard on the third picture and the tune after the last") {
  GIVEN("Version 1.0") {
    Knee knee(GameVersion::V10);

    THEN("The knee plays on every voice") {
      run(*knee.state, SAMPLE_FRAME);
      REQUIRE(knee.speaker.plays.empty());
      run(*knee.state, 1);
      REQUIRE(knee.speaker.plays ==
              std::vector<FakeSpeaker::Play>{{SAMPLE, Mixer::ALL_VOICES}});
    }

    THEN("The tune starts 20 frames after the last picture") {
      run(*knee.state, MUSIC_FRAME);
      REQUIRE(knee.speaker.musicStarts == 0);
      run(*knee.state, 1);
      REQUIRE(knee.speaker.musicStarts == 1);
      run(*knee.state, 10);
      REQUIRE(knee.speaker.tempos.empty());
    }

    THEN("The title follows once both screens have closed") {
      const Exit exit = runToExit(*knee.state, 1000);
      REQUIRE(exit.next == EngineStateId::TitleAndStory);
      REQUIRE(exit.frames == FRAMES);
    }
  }

  GIVEN("Version 1.2") {
    Knee knee(GameVersion::V12);

    THEN("The knee plays on two voices and the tune is slowed to 37") {
      run(*knee.state, SAMPLE_FRAME + 1);
      REQUIRE(knee.speaker.plays ==
              std::vector<FakeSpeaker::Play>{{SAMPLE, 0x3}});
      run(*knee.state, MUSIC_FRAME - SAMPLE_FRAME + 1);
      REQUIRE(knee.speaker.tempos.empty());
      run(*knee.state, 1);
      REQUIRE(knee.speaker.tempos == std::vector<int>{37});
    }
  }
}
