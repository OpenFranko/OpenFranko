#include "../../../../src/engine/street/ContinueScene.h"
#include "../../../../src/engine/street/StageFrame.h"
#include <catch2/catch_all.hpp>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street;

namespace {

constexpr int RO = 14;
constexpr int MACH_WAIT = 40;
constexpr int16_t JOY_LEFT = 4;
constexpr int16_t JOY_RIGHT = 8;
constexpr int16_t JOY_FIRE = 16;
constexpr uint8_t HAND_INK = 18;
constexpr uint8_t QUESTION_INK = 31;
constexpr uint32_t PURPLE = 0xFF770077u;
constexpr uint32_t GREY = 0xFFAAAAAAu;
constexpr uint32_t GOLD = 0xFFFFCC00u;

Picture box(int width, int height, uint8_t color) {
  return Picture{
      width, height, 0, 0,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height), color)};
}

class FakeHost : public StreetHost {
public:
  std::vector<std::pair<int, int>> spriteSets;

  std::vector<Picture> loadSpriteSet(int resource, int sampleBank) override {
    spriteSets.emplace_back(resource, sampleBank);
    std::vector<Picture> frames;
    frames.push_back(box(48, 23, HAND_INK));
    for (int image = 2; image <= 40; ++image) {
      frames.push_back(box(10, 15, 1));
    }
    frames.push_back(box(128, 40, QUESTION_INK));
    return frames;
  }

  Picture loadPicture(int) override { return box(320, 256, 0); }

  effects::AmigaPalette loadPalette(int) override { return {}; }

  std::vector<Picture> loadScenery(int) override { return {}; }

  LevelScript loadLevelScript(int) override { return LevelScript{}; }

  Picture loadPanelPicture(int) override { return box(304, 48, 7); }

  void loadMusic(int) override {}

  void playMusic() override {}

  void stopMusic() override {}

  void setMusicVolume(int) override {}

  void playSample(int, int, int) override {}

  void playSampleAt(int, int, int, int) override {}

  void setSampleLoop(bool) override {}

  int random(int) override { return 0; }
};

struct Choice {
  FakeHost host;
  GameSession session;
  ContinueScene scene{host, session};

  void run(int frames, int16_t joystick = 0) {
    for (int frame = 0; frame < frames; ++frame) {
      scene.advance(joystick);
    }
  }

  int16_t handX() const { return scene.bobs().x(ContinueScene::HAND); }

  uint16_t handImage() const {
    return static_cast<uint16_t>(scene.bobs().image(ContinueScene::HAND));
  }

  uint32_t pixel(int x, int y) const {
    std::vector<uint32_t> frame;
    scene.compose(frame);
    return frame[static_cast<std::size_t>(y * ContinueScene::WIDTH + x)];
  }
};

} // namespace

SCENARIO("The continue screen is drawn on the cleared hiscore screen") {
  GIVEN("The first frame of state 06") {
    Choice choice;
    choice.run(1);

    THEN("The letter set is reused, with no samples this time") {
      REQUIRE(choice.host.spriteSets ==
              std::vector<std::pair<int, int>>{{0x35, 0}});
    }

    THEN("Colour 0 is purple and only the text and hand colours are lit") {
      const effects::AmigaPalette &palette = choice.scene.palette();
      REQUIRE(palette[0] == 0x707);
      REQUIRE(palette[18] == 0xAAA);
      REQUIRE(palette[24] == 0xDDD);
      REQUIRE(palette[29] == 0x769);
      REQUIRE(palette[30] == 0xB95);
      REQUIRE(palette[31] == 0xFC0);
      REQUIRE(palette[1] == 0x000);
      REQUIRE(palette[28] == 0x000);
      REQUIRE(choice.pixel(0, 0) == PURPLE);
    }

    THEN("RESTART ? is pasted by its corner at 96,96") {
      REQUIRE(choice.pixel(96, 96) == GOLD);
      REQUIRE(choice.pixel(95, 96) == PURPLE);
      REQUIRE(choice.pixel(223, 135) == GOLD);
      REQUIRE(choice.pixel(224, 135) == PURPLE);
    }

    THEN("The hand points at TAK from 48,124 and continue is the default") {
      REQUIRE(choice.handX() == 48);
      REQUIRE(choice.scene.bobs().y(ContinueScene::HAND) == 124);
      REQUIRE(choice.handImage() == 1);
      REQUIRE(choice.pixel(48, 124) == GREY);
      REQUIRE(choice.pixel(47, 124) == PURPLE);
      REQUIRE(choice.scene.isContinueChosen());
    }
  }
}

SCENARIO("Right points the mirrored hand back at NIE") {
  GIVEN("The joystick pushed right") {
    Choice choice;
    choice.run(1);
    choice.run(1, JOY_RIGHT);

    THEN("Image $8001 at 268 is drawn to the left of x by its whole width") {
      REQUIRE(choice.handX() == 268);
      REQUIRE(choice.handImage() == 0x8001);
      REQUIRE(choice.pixel(220, 124) == GREY);
      REQUIRE(choice.pixel(267, 124) == GREY);
      REQUIRE(choice.pixel(268, 124) == PURPLE);
      REQUIRE_FALSE(choice.scene.isContinueChosen());
    }

    WHEN("It is pushed left again") {
      choice.run(1, JOY_LEFT);

      THEN("The hand is back at TAK") {
        REQUIRE(choice.handX() == 48);
        REQUIRE(choice.handImage() == 1);
        REQUIRE(choice.scene.isContinueChosen());
      }
    }
  }
}

SCENARIO("Fire waggles the hand through MACH, then the choice is taken") {
  GIVEN("A player who died on stage 1") {
    Choice choice;
    choice.session.stageReached = 1;
    choice.session.registers[RO] = -1;
    choice.run(3);

    WHEN("TAK is fired") {
      choice.run(1, JOY_FIRE);
      std::vector<int> path;
      for (int frame = 0; frame < 21; ++frame) {
        choice.run(1, JOY_RIGHT);
        path.push_back(choice.handX());
      }

      THEN("RACZKA's loop moves 4 px out and back four times, then rests") {
        REQUIRE(path == std::vector<int>{50, 52, 50, 48, 48, 50, 52,
                                         50, 48, 48, 50, 52, 50, 48,
                                         48, 50, 52, 50, 48, 48, 48});
      }

      THEN("The joystick is no longer read during Wait 40") {
        REQUIRE(choice.scene.isContinueChosen());
      }

      THEN("After Wait 40 the run resumes one stage back from ETAP") {
        choice.run(MACH_WAIT - 22);
        REQUIRE(choice.scene.outcome() == ContinueScene::Outcome::Choosing);
        REQUIRE(choice.scene.isShown());
        choice.run(1);
        REQUIRE(choice.scene.outcome() == ContinueScene::Outcome::Continue);
        REQUIRE(choice.session.stageReached == 0);
        REQUIRE(choice.session.registers[RO] == 0);
        REQUIRE_FALSE(choice.scene.isShown());
        REQUIRE_FALSE(choice.scene.bobs().isActive(ContinueScene::HAND));
      }
    }

    WHEN("NIE is fired") {
      choice.run(1, JOY_RIGHT);
      choice.run(1, JOY_FIRE);
      choice.run(MACH_WAIT);

      THEN("It is back to the menu with the stage left alone") {
        REQUIRE(choice.scene.outcome() == ContinueScene::Outcome::NewGame);
        REQUIRE(choice.session.stageReached == 1);
        REQUIRE(choice.session.registers[RO] == -1);
      }
    }
  }
}
