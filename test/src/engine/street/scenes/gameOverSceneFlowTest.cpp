#include "../../../../../src/engine/street/scenes/GameOverScene.h"

#include "../../../../../src/engine/street/ui/StageFrame.h"
#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../core/box.h"
#include "FakeStreetHost.h"
#include "SceneRunner.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <vector>

using namespace openfranko::src::engine::street::scenes;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::src::engine::street::ui;
using namespace openfranko::src::engine::street::core;
using namespace openfranko::test::src::engine::street::scenes;
using namespace openfranko::test::src::engine::street::core;
using namespace openfranko::src::systems::graphics;
using namespace openfranko::src::systems::input;

namespace {

constexpr uint8_t INK = 1;
constexpr int FRAME_LIMIT = 3000;

class FakeHost : public FakeStreetHost {
public:
  std::vector<Picture> loadSpriteSet(int resource, int sampleBank) override {
    spriteSets.emplace_back(resource, sampleBank);
    std::vector<Picture> frames;
    for (int i = 1; i <= 5; ++i) {
      frames.push_back(box(16, 4 * i, 7, 4 * i - 1, INK));
    }
    frames.push_back(box(144, 111, 0, 0, INK));
    return frames;
  }

  Picture loadPicture(int resource) override {
    pictures.push_back(resource);
    return box(GameOverScene::PICTURE_WIDTH, GameOverScene::PICTURE_HEIGHT, 0,
               0, 0);
  }
};

struct Graveyard : SceneRunner<Graveyard> {
  FakeHost host;
  GameSession session;
  GameOverScene scene{host, session};

  Graveyard() { session.border = STAGE_BORDER; }
};

} // namespace

SCENARIO("Sprites can be switched once the graveyard is open") {
  GIVEN("The graveyard panning, its bobs drawn into the picture") {
    Graveyard graveyard;
    graveyard.runUntil([&] { return graveyard.scene.isPanning(); },
                       FRAME_LIMIT);
    graveyard.run(10);

    THEN("Its layer carries no sprites") {
      REQUIRE_FALSE(graveyard.scene.output().layers.front().carriesSprites);
    }

    WHEN("Sprites are switched on") {
      graveyard.scene.showSprites(true);
      graveyard.run(2);

      THEN("The title and the hand ride on the layer as sprites") {
        const Display shown = graveyard.scene.output();
        REQUIRE(shown.layers.front().carriesSprites);
        REQUIRE(shown.layers.front().sprites.size() == 2);
      }

      AND_WHEN("They are switched off again") {
        graveyard.scene.showSprites(false);

        THEN("They are stamped back into the picture") {
          const Display shown = graveyard.scene.output();
          REQUIRE_FALSE(shown.layers.front().carriesSprites);
          REQUIRE(shown.layers.front().sprites.empty());
        }
      }
    }
  }
}

SCENARIO("Nothing moves once game over has finished") {
  GIVEN("A game over clicked through to its end") {
    Graveyard graveyard;
    const int frames = graveyard.runUntil(
        [&] { return graveyard.scene.isFinished(); }, FRAME_LIMIT, JOY_FIRE);
    const int stops = graveyard.host.musicStops;
    const std::size_t volumes = graveyard.host.volumes.size();
    const int offset = graveyard.scene.offset();

    WHEN("More frames come with fire held") {
      graveyard.run(50, JOY_FIRE);

      THEN("No sound changes, the screen stays closed and the pan stays put") {
        REQUIRE(frames > 0);
        REQUIRE(graveyard.scene.isFinished());
        REQUIRE(graveyard.host.musicStops == stops);
        REQUIRE(graveyard.host.volumes.size() == volumes);
        REQUIRE_FALSE(graveyard.scene.isShown());
        REQUIRE(graveyard.scene.offset() == offset);
        REQUIRE(graveyard.scene.output().layers.empty());
      }
    }
  }
}
