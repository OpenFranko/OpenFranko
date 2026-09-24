#include "../../../../src/engine/street/GameOverScene.h"
#include "../../../../src/engine/effects/Rainbow.h"
#include "../../../../src/engine/street/StageFrame.h"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <functional>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street;

namespace {

constexpr int OBJECTS = 0x36;
constexpr int GRAVEYARD = 0x3BB;
constexpr int TUNE = 0x262;
constexpr int OPEN_FRAME = 1 + GameOverScene::FILES * LoadingMock::FILE_FRAMES;
constexpr int OPENED_FRAME = OPEN_FRAME + 1 + 3;
constexpr uint32_t GREY = 0xFF555555u;
constexpr uint32_t BLACK = 0xFF000000u;
constexpr uint32_t RED = 0xFFFF0000u;
constexpr int PAN_FRAMES = 545;
constexpr int16_t JOY_FIRE = 16;
constexpr uint8_t SILHOUETTE = 1;
constexpr uint8_t TITLE_INK = 2;

Picture box(int width, int height, int hotX, int hotY, uint8_t color) {
  return Picture{
      width, height, hotX, hotY,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height), color)};
}

Picture graveyard() {
  Picture picture =
      box(GameOverScene::PICTURE_WIDTH, GameOverScene::PICTURE_HEIGHT, 0, 0, 0);
  for (int y = 1; y < GameOverScene::PICTURE_HEIGHT; y += 2) {
    for (int x = 0; x < 40; ++x) {
      picture.pixels[static_cast<std::size_t>(y * GameOverScene::PICTURE_WIDTH +
                                              x)] = SILHOUETTE;
    }
  }
  return picture;
}

class FakeHost : public StreetHost {
public:
  std::vector<std::pair<int, int>> spriteSets;
  std::vector<int> pictures;
  std::vector<int> music;
  std::vector<int> volumes;
  int musicStarts = 0;
  int musicStops = 0;

  std::vector<Picture> loadSpriteSet(int resource, int sampleBank) override {
    spriteSets.emplace_back(resource, sampleBank);
    std::vector<Picture> frames;
    for (int i = 1; i <= 5; ++i) {
      frames.push_back(box(16, 4 * i, 7, 4 * i - 1, SILHOUETTE));
    }
    frames.push_back(box(144, 111, 0, 0, TITLE_INK));
    return frames;
  }

  Picture loadPicture(int resource) override {
    pictures.push_back(resource);
    return graveyard();
  }

  effects::AmigaPalette loadPalette(int) override { return {}; }

  std::vector<Picture> loadScenery(int) override { return {}; }

  LevelScript loadLevelScript(int) override { return LevelScript{}; }

  EndingCredits loadEndingCredits() override { return {}; }

  Picture loadPanelPicture(int) override { return box(304, 48, 0, 0, 7); }

  void loadMusic(int resource) override { music.push_back(resource); }

  void playMusic() override { ++musicStarts; }

  void stopMusic() override { ++musicStops; }

  void setMusicVolume(int volume) override { volumes.push_back(volume); }
  void setMusicTempo(int) override {}

  void playSample(int, int, int) override {}

  void playSampleAt(int, int, int, int) override {}

  void setSampleLoop(bool) override {}

  int random(int) override { return 0; }
};

struct Graveyard {
  FakeHost host;
  GameOverScene scene{host};

  void run(int frames, int16_t joystick = 0) {
    for (int frame = 0; frame < frames; ++frame) {
      scene.advance(joystick);
    }
  }

  int runUntil(const std::function<bool()> &done, int limit,
               int16_t joystick = 0) {
    for (int frame = 0; frame < limit; ++frame) {
      scene.advance(joystick);
      if (done()) {
        return frame + 1;
      }
    }
    return -1;
  }

  uint32_t pixel(int x, int y) const {
    std::vector<uint32_t> frame;
    scene.compose(frame);
    return frame[static_cast<std::size_t>(y * GameOverScene::WIDTH + x)];
  }
};

} // namespace

SCENARIO("Game over loads its three files while the screens are closed") {
  GIVEN("The first frame after the stage gave up") {
    Graveyard graveyard;
    graveyard.run(1);

    THEN("ERA has stopped the music and only the stage's grey border shows") {
      REQUIRE(graveyard.host.musicStops == 1);
      REQUIRE_FALSE(graveyard.scene.isShown());
      REQUIRE(graveyard.pixel(0, 0) == 0xFF555555u);
      REQUIRE(graveyard.pixel(367, 255) == 0xFF555555u);
    }

    THEN("The objects come first, then the picture, then the tune") {
      REQUIRE(graveyard.host.spriteSets ==
              std::vector<std::pair<int, int>>{{OBJECTS, 0}});
      REQUIRE(graveyard.host.pictures.empty());
      graveyard.run(LoadingMock::FILE_FRAMES);
      REQUIRE(graveyard.host.pictures == std::vector<int>{GRAVEYARD});
      REQUIRE(graveyard.host.music.empty());
      graveyard.run(LoadingMock::FILE_FRAMES);
      REQUIRE(graveyard.host.music == std::vector<int>{TUNE});
      REQUIRE(graveyard.host.musicStarts == 0);
    }
  }

  GIVEN("The frame the last file is in") {
    Graveyard graveyard;
    graveyard.run(OPEN_FRAME);

    THEN("Music 1 starts, then Unpack 9 To 0 waits a VBL before linking the "
         "screen") {
      REQUIRE(graveyard.host.musicStarts == 1);
      REQUIRE(graveyard.host.volumes == std::vector<int>{63});
      REQUIRE_FALSE(graveyard.scene.isShown());
      graveyard.run(1);
      REQUIRE(graveyard.scene.isShown());
      REQUIRE(graveyard.pixel(300, 100) == GREY);
    }

    THEN("The screen shows at the next copper rebuild, black in its own "
         "palette, through Double Buffer's three VBLs") {
      graveyard.run(2);
      REQUIRE(graveyard.pixel(300, 100) == BLACK);
      REQUIRE(graveyard.pixel(0, 0) == BLACK);
      graveyard.run(1);
      REQUIRE(graveyard.pixel(300, 100) == BLACK);
      REQUIRE_FALSE(graveyard.scene.isPanning());
      graveyard.run(1);
      REQUIRE(graveyard.scene.isPanning());
      REQUIRE(graveyard.scene.offset() == 0);
    }
  }

  GIVEN("The frame the pan begins") {
    Graveyard graveyard;
    graveyard.run(OPENED_FRAME);

    THEN("Colour 2 is red, colour 9 dark grey and the rest black") {
      const auto &palette = graveyard.scene.palette();
      REQUIRE(palette[0] == 0x000);
      REQUIRE(palette[1] == 0x000);
      REQUIRE(palette[2] == 0xF00);
      REQUIRE(palette[9] == 0x222);
    }

    THEN("The rainbow starts at line 28, so the screen's line 45 shows entry "
         "113 and it stops after line 267") {
      const effects::AmigaPalette table = effects::rainbowTable(
          1000, "(8,-1,15)(16,1,15)", "", "(8,1,15)(16,-1,15)");
      REQUIRE(graveyard.pixel(300, 0) == toArgb(table[113]));
      REQUIRE(graveyard.pixel(300, 100) == toArgb(table[213]));
      REQUIRE(graveyard.pixel(300, 222) == toArgb(table[335]));
      REQUIRE(graveyard.pixel(300, 223) == 0xFF000000u);
      REQUIRE(graveyard.pixel(10, 1) == 0xFF000000u);
    }

    THEN("The title is pinned at X Screen(200) and the hand waits at 820") {
      REQUIRE(graveyard.scene.bobs().x(1) == 104);
      REQUIRE(graveyard.scene.bobs().y(1) == 80);
      REQUIRE(graveyard.scene.bobs().image(1) == 6);
      REQUIRE(graveyard.scene.bobs().x(2) == 820);
      REQUIRE(graveyard.scene.bobs().y(2) == 209);
    }

    THEN("BACK[0]'s test draws the bobs into the hidden buffer, shown a VBL "
         "later") {
      REQUIRE(graveyard.pixel(104, 80) != RED);
      graveyard.run(1);
      REQUIRE(graveyard.pixel(104, 80) == RED);
    }
  }
}

SCENARIO("The picture pans 5 px every 4 frames under the pinned title") {
  GIVEN("The open graveyard") {
    Graveyard graveyard;
    graveyard.run(OPENED_FRAME);

    WHEN("It pans for forty frames") {
      std::vector<int> offsets;
      for (int frame = 0; frame < 40; ++frame) {
        graveyard.run(1);
        offsets.push_back(graveyard.scene.offset());
      }
      int left = -1;
      for (int x = 0; x < GameOverScene::WIDTH; ++x) {
        if (graveyard.pixel(x, 80) == RED) {
          left = x;
          break;
        }
      }

      THEN("The title trails its pin by the last step: the offset reaches "
           "the copper a VBL after Screen Offset, the bob two VBLs after Bob") {
        REQUIRE(offsets[38] - offsets[37] > 0);
        REQUIRE(left == 104 - (offsets[38] - offsets[37]));
      }
    }

    WHEN("Three hundred frames have passed") {
      graveyard.run(300);

      THEN("The offset and the title have moved together") {
        REQUIRE(graveyard.scene.offset() == 375);
        REQUIRE(graveyard.scene.bobs().x(1) == 479);
        REQUIRE(graveyard.pixel(104, 80) == 0xFFFF0000u);
      }
    }

    WHEN("The hand has run through its animation") {
      std::vector<int> images;
      for (int frame = 0; frame < 81; ++frame) {
        graveyard.run(1);
        const int image = graveyard.scene.bobs().image(2);
        if (images.empty() || images.back() != image) {
          images.push_back(image);
        }
      }

      THEN("It rises through images 1 to 5 and sinks back to 1") {
        REQUIRE(images == std::vector<int>{1, 2, 3, 4, 5, 3, 2, 1});
      }
    }

    WHEN("The pan reaches 680 and the copper has taken it") {
      graveyard.run(PAN_FRAMES - 1);
      const int offset = graveyard.scene.offset();
      graveyard.run(1);

      THEN("The last 40 columns show the start of the next row") {
        REQUIRE(offset == GameOverScene::PAN_END);
        REQUIRE(graveyard.pixel(367, 0) == 0xFF000000u);
        REQUIRE(graveyard.pixel(367, 1) != 0xFF000000u);
        REQUIRE(graveyard.pixel(327, 0) != 0xFF000000u);
      }
    }
  }
}

SCENARIO("KLIKER, Fade 5 and SCICH close the scene") {
  GIVEN("The pan has ended") {
    Graveyard graveyard;
    graveyard.run(OPENED_FRAME + PAN_FRAMES - 1);
    REQUIRE(graveyard.scene.isPanning());

    WHEN("Nobody touches the joystick") {
      const int fading = graveyard.runUntil(
          [&] { return graveyard.host.volumes.size() > 1; }, 1000);

      THEN("The music starts to fade on the 401st frame") {
        REQUIRE(fading == 401);
        REQUIRE(graveyard.host.volumes.back() == 63);
      }
    }

    WHEN("Fire is pressed") {
      const int fading = graveyard.runUntil(
          [&] { return graveyard.host.volumes.size() > 1; }, 1000, JOY_FIRE);

      THEN("The wait ends at once") { REQUIRE(fading == 1); }

      AND_WHEN("The fades have run") {
        graveyard.run(71);

        THEN("Fade 5 has turned the title black but the rainbow stays") {
          REQUIRE(graveyard.scene.palette()[2] == 0x000);
          REQUIRE(graveyard.pixel(300, 0) != 0xFF000000u);
        }
      }

      AND_WHEN("It runs to the end") {
        const int finished = graveyard.runUntil(
            [&] { return graveyard.scene.isFinished(); }, 1000);

        THEN("SCICH steps the volume from 63 down to 0 before Music Off") {
          const std::vector<int> &volumes = graveyard.host.volumes;
          REQUIRE(volumes.size() == 1 + 64 + 1);
          for (int i = 0; i < 64; ++i) {
            REQUIRE(volumes[static_cast<std::size_t>(1 + i)] == 63 - i);
          }
          REQUIRE(volumes.back() == 63);
          REQUIRE(graveyard.host.musicStops == 2);
        }

        THEN("Wait 100 follows, then the screens close to a black border") {
          REQUIRE(finished == 63 + 1 + 100);
          REQUIRE_FALSE(graveyard.scene.isShown());
          REQUIRE(graveyard.pixel(300, 0) == 0xFF000000u);
        }
      }
    }
  }
}
