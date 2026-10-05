#include "../../../../../src/engine/street/scenes/EndingScene.h"

#include "../../../../../src/engine/street/ui/StageFrame.h"
#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../core/box.h"
#include "FakeStreetHost.h"
#include "SceneRunner.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street::scenes;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::src::engine::street::ui;
using namespace openfranko::src::engine::street::core;
using namespace openfranko::test::src::engine::street::scenes;
using namespace openfranko::test::src::engine::street::core;
using namespace openfranko::src::systems::graphics;
using namespace openfranko::src::engine::amal;
using namespace openfranko::src::systems::input;

namespace {

constexpr int DANCE_SET = 0x38;
constexpr int DANCE_IMAGES = 101;
constexpr int STILL_IMAGES = 26;
constexpr int STEPPED_FILES = 3;
constexpr int FRAME_LIMIT = 20000;

template <typename Load, typename... Targets> class Stepped : public Load {
public:
  Stepped(std::unique_ptr<Load> whole, int steps)
      : m_whole(std::move(whole)), m_steps(steps) {}

  bool step(Targets... targets) override {
    if (--m_steps > 0) {
      return false;
    }
    return m_whole->step(targets...);
  }

private:
  std::unique_ptr<Load> m_whole;
  int m_steps;
};

using SteppedSprites = Stepped<StreetHost::SpriteSetLoad, ImageBank &>;
using SteppedPicture =
    Stepped<StreetHost::PictureLoad, Picture &, effects::color::AmigaPalette *>;
using SteppedCredits = Stepped<StreetHost::CreditsLoad, EndingCredits &>;

EndingCredits twoPages() {
  EndingCredits credits;
  credits.pages.push_back({{{"AB", 16}}, 0});
  credits.pages.push_back({{{"CD", 16}}, 10});
  return credits;
}

class FakeHost : public FakeStreetHost {
public:
  int fileSteps = 1;
  int creditSteps = 1;

  std::vector<Picture> loadSpriteSet(int resource, int sampleBank) override {
    spriteSets.emplace_back(resource, sampleBank);
    const int images = resource == DANCE_SET ? DANCE_IMAGES : STILL_IMAGES;
    std::vector<Picture> frames;
    for (int image = 1; image <= images; ++image) {
      frames.push_back(box(16, 16, 0, 0, static_cast<uint8_t>(image)));
    }
    return frames;
  }

  Picture loadPicture(int resource) override {
    pictures.push_back(resource);
    return box(320, 256, 0, 0, 7);
  }

  effects::color::AmigaPalette loadPalette(int) override {
    return effects::color::AmigaPalette(32, 0x123);
  }

  EndingCredits loadEndingCredits() override { return twoPages(); }

  std::unique_ptr<SpriteSetLoad> beginSpriteSet(int resource, int sampleBank,
                                                int base, int steps) override {
    return std::make_unique<SteppedSprites>(
        FakeStreetHost::beginSpriteSet(resource, sampleBank, base, steps),
        fileSteps);
  }

  std::unique_ptr<PictureLoad> beginPicture(int resource) override {
    return std::make_unique<SteppedPicture>(
        FakeStreetHost::beginPicture(resource), fileSteps);
  }

  std::unique_ptr<CreditsLoad> beginEndingCredits() override {
    return std::make_unique<SteppedCredits>(
        FakeStreetHost::beginEndingCredits(), creditSteps);
  }
};

struct Ending : SceneRunner<Ending> {
  FakeHost host;
  GameSession session;
  EndingScene scene;

  Ending() : scene(host, session) {
    session.registers[RO] = 3;
    IndexedSurface shown(320, 222);
    shown.fill(3);
    BossExit exit{DoubleBuffer(shown), levelPalette(false), 47, 0,
                  IndexedSurface(304, 48)};
    session.bossExit.emplace(std::move(exit));
  }

  int clickUntil(const std::function<bool()> &done) {
    for (int frame = 0; frame < FRAME_LIMIT; ++frame) {
      scene.advance(frame % 2 == 0 ? JOY_FIRE : 0);
      if (done()) {
        return frame + 1;
      }
    }
    return -1;
  }
};

} // namespace

SCENARIO("Ending files read in many steps hold the loads until they are in") {
  GIVEN("Two endings, one whose sets and picture take 70 steps each") {
    constexpr int STEPS = 70;
    Ending whole;
    Ending stepped;
    stepped.host.fileSteps = STEPS;

    WHEN("Both load") {
      const int wholeLoaded =
          whole.runUntil([&] { return !whole.scene.isLoading(); }, 1000);
      const int steppedLoaded =
          stepped.runUntil([&] { return !stepped.scene.isLoading(); }, 1000);

      THEN("Each stepped file outlasts its 51 frames by the steps left") {
        REQUIRE(wholeLoaded > 0);
        REQUIRE(steppedLoaded ==
                wholeLoaded +
                    STEPPED_FILES * (STEPS - (LoadingQueue::FILE_FRAMES + 1)));
      }

      THEN("The still that follows is the same picture") {
        whole.runUntil([&] { return whole.scene.isShowingStill(); }, 1000);
        stepped.runUntil([&] { return stepped.scene.isShowingStill(); }, 1000);
        whole.run(100);
        stepped.run(100);
        REQUIRE(stepped.scene.screen(0).pixels() ==
                whole.scene.screen(0).pixels());
        REQUIRE(stepped.scene.palette(0) == whole.scene.palette(0));
      }
    }
  }
}

SCENARIO("Credits still being read when the text screen opens are read "
         "there in one go") {
  GIVEN("Credits that would take 100000 frames to read") {
    Ending ending;
    ending.host.creditSteps = 100000;

    WHEN("The ending is clicked through to the text screen") {
      const int shown =
          ending.clickUntil([&] { return ending.scene.isShowingCredits(); });

      THEN("Both pages are there and the first is up") {
        REQUIRE(shown > 0);
        REQUIRE(shown < 100000);
        REQUIRE(ending.scene.page() == 0);
        REQUIRE(ending.clickUntil([&] { return ending.scene.page() == 1; }) >
                0);
      }
    }
  }
}

SCENARIO("Sprites can be switched while the dancer is on screen") {
  GIVEN("The ending clicked through to the break-dance, the dancer walked "
        "in with sprites off") {
    Ending ending;
    ending.clickUntil([&] { return ending.scene.isShown(1); });
    ending.run(200);
    const auto dancerLayer = [&ending]() {
      const Display upcoming = ending.scene.upcomingOutput();
      for (const Layer &layer : upcoming.layers) {
        if (layer.top + EndingScene::DISPLAY_LINE > 100) {
          return layer;
        }
      }
      return Layer{};
    };

    THEN("The dancer is drawn into its screen") {
      REQUIRE(dancerLayer().pixels != nullptr);
      REQUIRE_FALSE(dancerLayer().carriesSprites);
    }

    WHEN("Sprites are switched on for the dancer") {
      ending.scene.showSprites(false, true);
      ending.run(2);

      THEN("Its bobs ride on the screen's layer as sprites") {
        REQUIRE(dancerLayer().carriesSprites);
        REQUIRE_FALSE(dancerLayer().sprites.empty());
      }

      AND_WHEN("They are switched off again") {
        ending.scene.showSprites(false, false);

        THEN("The layer carries no sprites") {
          REQUIRE_FALSE(dancerLayer().carriesSprites);
          REQUIRE(dancerLayer().sprites.empty());
        }
      }
    }
  }
}

SCENARIO("Nothing moves once the ending has finished") {
  GIVEN("An ending clicked through to its end") {
    Ending ending;
    const int frames =
        ending.clickUntil([&] { return ending.scene.isFinished(); });
    const int stops = ending.host.musicStops;
    const std::size_t volumes = ending.host.volumes.size();
    const Display shown = ending.scene.output();

    WHEN("More frames come") {
      ending.run(50, JOY_FIRE);

      THEN("No sound changes and the output stays as it was") {
        REQUIRE(frames > 0);
        REQUIRE(ending.scene.isFinished());
        REQUIRE(ending.host.musicStops == stops);
        REQUIRE(ending.host.volumes.size() == volumes);
        Display filled;
        ending.scene.output(filled);
        REQUIRE(filled.layers.size() == shown.layers.size());
        REQUIRE(filled.border == shown.border);
      }

      THEN("The upcoming output is the one shown") {
        const Display upcoming = ending.scene.upcomingOutput();
        REQUIRE(upcoming.layers.size() == shown.layers.size());
        for (std::size_t layer = 0; layer < shown.layers.size(); ++layer) {
          REQUIRE(upcoming.layers[layer].pixels == shown.layers[layer].pixels);
        }
      }
    }
  }
}
