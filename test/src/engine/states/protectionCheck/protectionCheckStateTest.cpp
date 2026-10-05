#include "../../../../../src/engine/states/protectionCheck/ProtectionCheckState.h"

#include "../../../../../src/engine/InkeyBuffer.h"
#include "../../../../../src/engine/street/ui/LoadingQueue.h"
#include "../../../../../src/engine/street/ui/StageFrame.h"
#include "../../../systems/audio/FakeSpeaker.h"
#include "../../../systems/graphics/FakeMonitor.h"
#include "../../assets/FakeFiles.h"
#include "../StateRunner.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::states;
using namespace openfranko::src::engine::states::protectionCheck;
using namespace openfranko::src::engine::street::ui;
using namespace openfranko::src::systems::graphics;
using namespace openfranko::test::src::engine::assets;
using namespace openfranko::test::src::engine::states;
using namespace openfranko::test::src::systems::audio;
using namespace openfranko::test::src::systems::graphics;

namespace {

constexpr auto CARDS = "assets/0384/0384_cards.bin";
constexpr auto QUESTION = "assets/03C1.bmp";
constexpr auto FAILURE = "assets/03C2.bmp";
constexpr int ANSWER_FRAMES = 20;
constexpr uint16_t PAPER = 0xDDD;

IndexedBitmap picture(int height) {
  IndexedBitmap bitmap;
  bitmap.width = 320;
  bitmap.height = height;
  bitmap.pixels.assign(static_cast<std::size_t>(320) * height, 1);
  bitmap.palette.assign(32, 0x000);
  bitmap.palette[1] = PAPER;
  return bitmap;
}

FakeFiles cardFiles() {
  FakeFiles files;
  files.contents[CARDS] = std::vector<uint8_t>(200, 0);
  files.bitmaps[QUESTION] = picture(200);
  files.bitmaps[FAILURE] = picture(256);
  return files;
}

struct Check {
  explicit Check(
      ProtectionCheckState::Check kind = ProtectionCheckState::Check::Title)
      : state(monitor, speaker, files, keyboard, kind) {
    keyboard.permit();
  }

  void answer(char letter) {
    keyboard.press(letter);
    run(state, ANSWER_FRAMES);
  }

  FakeMonitor monitor;
  FakeSpeaker speaker;
  FakeFiles files = cardFiles();
  InkeyBuffer keyboard;
  ProtectionCheckState state;
};

} // namespace

SCENARIO("The code cards come from the files") {
  GIVEN("The title check") {
    Check check;

    THEN("The cards are read on entry and the question when it is asked") {
      REQUIRE(check.files.loaded == std::vector<std::string>{CARDS});
      run(check.state, 2);
      REQUIRE(check.files.loaded == std::vector<std::string>{CARDS, QUESTION});
    }
  }

  GIVEN("No card file") {
    FakeMonitor monitor;
    FakeSpeaker speaker;
    FakeFiles files;
    InkeyBuffer keyboard;

    THEN("The check cannot start") {
      REQUIRE_THROWS_WITH(
          ProtectionCheckState(monitor, speaker, files, keyboard),
          "Failed to open code cards: assets/0384/0384_cards.bin");
    }
  }
}

SCENARIO("The title check wants both cards right") {
  GIVEN("Cards whose every cell is A") {
    Check check;
    run(check.state, 2);

    WHEN("Both answers are right, typed in either case") {
      check.answer('a');
      check.keyboard.press('A');
      const Exit exit = runToExit(check.state, 100);

      THEN("The scores follow") {
        REQUIRE(exit.next == EngineStateId::HighScore);
        REQUIRE(check.files.loaded ==
                std::vector<std::string>{CARDS, QUESTION, QUESTION});
        REQUIRE(check.state.check().isFinished());
        REQUIRE(check.state.check().isPassed());
      }
    }

    WHEN("One answer is wrong") {
      check.answer('B');
      check.keyboard.press('A');
      const Exit exit = runToExit(check.state, 1000);

      THEN("The failure picture stays up with the music stopped") {
        REQUIRE_FALSE(exit.next.has_value());
        REQUIRE(check.files.wasLoaded(FAILURE));
        REQUIRE(check.speaker.musicStops == 1);
        REQUIRE_FALSE(check.state.isEnteringText());
        REQUIRE(check.state.check().isFinished());
        REQUIRE_FALSE(check.state.check().isPassed());
      }
    }

    WHEN("Keys that are not answers are typed") {
      check.answer('Z');
      check.answer('1');

      THEN("The question is still asked") {
        REQUIRE(check.files.loaded ==
                std::vector<std::string>{CARDS, QUESTION});
      }
    }
  }
}

SCENARIO("Typed text is wanted only while a question is up") {
  GIVEN("The title check") {
    Check check;

    THEN("Nothing is asked before the first question") {
      REQUIRE_FALSE(check.state.isEnteringText());
    }

    WHEN("The first question is up") {
      run(check.state, 2);

      THEN("An answer is asked for") { REQUIRE(check.state.isEnteringText()); }

      AND_WHEN("It is answered") {
        check.keyboard.press('A');
        run(check.state, 1);

        THEN("Nothing is asked while the question closes") {
          REQUIRE_FALSE(check.state.isEnteringText());
        }
      }
    }
  }
}

SCENARIO("The stage 3 check lets the first right answer through") {
  GIVEN("The check after stage 2") {
    Check check(ProtectionCheckState::Check::Stage3);

    THEN("Its two files load on the stage's grey") {
      run(check.state, 2 * LoadingQueue::FILE_FRAMES);
      REQUIRE(check.monitor.pixel(0, 0) == toArgb(STAGE_BORDER));
      REQUIRE(check.files.loaded == std::vector<std::string>{CARDS});
    }

    WHEN("A wrong answer comes before a right one") {
      run(check.state, 2 * LoadingQueue::FILE_FRAMES + 2);
      check.answer('C');
      check.keyboard.press('A');
      const Exit exit = runToExit(check.state, 100);

      THEN("Stage 3 follows") { REQUIRE(exit.next == EngineStateId::Level3); }
    }
  }
}
