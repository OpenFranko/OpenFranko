#include "../../../../src/engine/street/EndingScene.h"
#include "../../../../src/engine/street/StageFrame.h"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street;

namespace {

constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RN = 13;
constexpr int RO = 14;

constexpr int DANCE_SET = 0x38;
constexpr int STILL_SET = 0x37;
constexpr int STILL_PICTURE = 0x3BD;
constexpr int ENDING_TUNE = 0x25F;
constexpr int DANCE_IMAGES = 101;
constexpr int STILL_IMAGES = 26;
constexpr int STILL_MARK = 150;
constexpr uint8_t PICTURE_COLOR = 7;
constexpr uint8_t STRIP_COLOR = 7;
constexpr uint8_t WAIT_WORD_COLOR = 5;
constexpr uint8_t STREET_COLOR = 3;
constexpr uint8_t BOB_COLOR = 9;
constexpr uint8_t SCORE_COLOR = 4;
constexpr int STAGE_TOP = 47;
constexpr int PANEL_ROW = 270 - EndingScene::DISPLAY_LINE;

constexpr int16_t JOY_RIGHT = 8;
constexpr int16_t JOY_FIRE = 16;

constexpr int LOADED = 3 + EndingScene::FILES * LoadingMock::FILE_FRAMES + 1;
constexpr int STILL_SHOWN = LOADED + 2 + 2 + 2;
constexpr int TEXT_BOX_SHOWN = STILL_SHOWN + 5 + 60 + 2;
constexpr int KLIKER_START = TEXT_BOX_SHOWN + 1;

constexpr int CREDIT_PAGES = 12;

EndingCredits syntheticCredits() {
  EndingCredits credits;
  credits.pages.push_back({{{"FGHIJK", 16}}, 0});
  credits.pages.push_back({{{"LM", 16}}, 10});
  for (int page = 2; page < CREDIT_PAGES; ++page) {
    credits.pages.push_back({{{"NOP", 0}, {"Q%R", 40}}, 10 * page});
  }
  return credits;
}

int beatsBefore(const EndingCredits &credits, int page) {
  int frames = 0;
  for (int i = 0; i < page; ++i) {
    frames += 300 + credits.pages[static_cast<std::size_t>(i)].beat;
  }
  return frames;
}

Picture box(int width, int height, int hotX, int hotY, uint8_t color) {
  return Picture{
      width, height, hotX, hotY,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height), color)};
}

effects::AmigaPalette picturePalette() {
  effects::AmigaPalette palette(32);
  for (std::size_t i = 0; i < palette.size(); ++i) {
    palette[i] = static_cast<effects::AmigaColor>((i * 0x123) & 0xFFF);
  }
  return palette;
}

class FakeHost : public StreetHost {
public:
  std::vector<std::pair<int, int>> spriteSets;
  std::vector<int> pictures;
  std::vector<int> music;
  int musicStarts = 0;
  int musicStops = 0;
  std::vector<int> volumes;

  std::vector<Picture> loadSpriteSet(int resource, int sampleBank) override {
    spriteSets.emplace_back(resource, sampleBank);
    std::vector<Picture> frames;
    if (resource == DANCE_SET) {
      for (int image = 1; image <= DANCE_IMAGES; ++image) {
        frames.push_back(box(16, 16, 0, 0, static_cast<uint8_t>(image)));
      }
    } else {
      for (int image = 1; image <= STILL_IMAGES; ++image) {
        frames.push_back(
            box(16, 16, 0, 0, static_cast<uint8_t>(STILL_MARK + image)));
      }
    }
    return frames;
  }

  Picture loadPicture(int resource) override {
    pictures.push_back(resource);
    return box(320, 256, 0, 0, PICTURE_COLOR);
  }

  effects::AmigaPalette loadPalette(int) override { return picturePalette(); }

  std::vector<Picture> loadScenery(int) override { return {}; }

  LevelScript loadLevelScript(int) override { return LevelScript{}; }

  EndingCredits credits = syntheticCredits();
  int creditLoads = 0;

  EndingCredits loadEndingCredits() override {
    ++creditLoads;
    return credits;
  }

  Picture loadPanelPicture(int part) override {
    if (part != 0) {
      return box(304, 40, 0, 0, SCORE_COLOR);
    }
    Picture strip = box(304, 48, 0, 0, STRIP_COLOR);
    std::fill(strip.pixels.begin() + 32 * 304, strip.pixels.end(),
              WAIT_WORD_COLOR);
    return strip;
  }

  void loadMusic(int resource) override { music.push_back(resource); }

  void playMusic() override { ++musicStarts; }

  void stopMusic() override { ++musicStops; }

  void setMusicVolume(int volume) override { volumes.push_back(volume); }

  void playSample(int, int, int) override {}

  void playSampleAt(int, int, int, int) override {}

  void setSampleLoop(bool) override {}

  int random(int limit) override { return limit; }
};

struct Ending {
  FakeHost host;
  GameSession session;
  EndingScene scene{host, session};

  Ending() {
    session.registers[RF] = 40;
    session.registers[RO] = 3;
    session.registers[RN] = 99;
    session.registers[RG] = 2;
    IndexedSurface shown(320, 222);
    shown.fill(STREET_COLOR);
    shown.clear(BOB_COLOR, 100, 100, 116, 116);
    BossExit exit{DoubleBuffer(shown), levelPalette(false), STAGE_TOP, 0,
                  IndexedSurface(304, 48)};
    exit.buffer.autoback([](IndexedSurface &surface) { surface.fill(0); });
    exit.panel.fill(SCORE_COLOR);
    session.bossExit.emplace(std::move(exit));
  }

  uint32_t pixel(int x, int row) const {
    std::vector<uint32_t> frame;
    scene.compose(frame);
    return frame[static_cast<std::size_t>(row * EndingScene::WIDTH + x)];
  }

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

  void reachLastWalkFrame() {
    run(KLIKER_START);
    run(1, JOY_FIRE);
    runUntil(
        [this] {
          return scene.bobs().image(2) == 26 && !scene.machine().isRunning(2);
        },
        3000);
  }

  void reachCredits() {
    reachLastWalkFrame();
    run(1, JOY_FIRE);
    runUntil([this] { return scene.isShowingCredits(); }, 200);
  }

  int panelPixel(int x, int y) const { return scene.panel()->pixel(x, y); }
};

} // namespace

SCENARIO("CONGRA clears the stage, stops the tune and loads four files") {
  GIVEN("The third boss just beaten") {
    Ending ending;
    EndingScene &scene = ending.scene;
    ending.run(1);

    THEN("Cls 0's first VBL still shows the boss's last frame, bobs and "
         "panel") {
      REQUIRE(scene.isLoading());
      REQUIRE(scene.isStageShown());
      REQUIRE(scene.border() == 0x555);
      REQUIRE(ending.host.musicStops == 0);
      REQUIRE_FALSE(ending.session.bossExit.has_value());
      const effects::AmigaPalette &colors = levelPalette(false);
      REQUIRE(ending.pixel(0, 0) == toArgb(colors[STREET_COLOR]));
      REQUIRE(ending.pixel(100, 100 + STAGE_TOP - EndingScene::DISPLAY_LINE) ==
              toArgb(colors[BOB_COLOR]));
      REQUIRE(ending.pixel(310, 0) == toArgb(0x555));
      REQUIRE(ending.panelPixel(101, 10) == SCORE_COLOR);
      REQUIRE(ending.pixel(101, PANEL_ROW) ==
              toArgb(panelPalette()[SCORE_COLOR]));
    }

    WHEN("The swap to the cleared buffer comes a VBL later") {
      ending.run(1);

      THEN("The stage is colour 0 for the last two VBLs, the panel still "
           "the boss's") {
        REQUIRE(ending.pixel(0, 0) == toArgb(levelPalette(false)[0]));
        REQUIRE(ending.pixel(100, 97) == toArgb(levelPalette(false)[0]));
        REQUIRE(ending.panelPixel(101, 10) == SCORE_COLOR);
        ending.run(1);
        REQUIRE(ending.pixel(100, 97) == toArgb(levelPalette(false)[0]));
        REQUIRE(ending.host.musicStops == 0);
      }
    }

    WHEN("The Cls stall of three frames has passed") {
      ending.run(3);

      THEN("ERA stops the boss's tune and LADUJ paints the strip for the "
           "dance set") {
        REQUIRE(ending.host.musicStops == 1);
        REQUIRE(ending.host.spriteSets ==
                std::vector<std::pair<int, int>>{{DANCE_SET, 0}});
        REQUIRE(ending.panelPixel(101, 10) == STRIP_COLOR);
      }
    }

    WHEN("All four files are in") {
      ending.run(LOADED - 1);

      THEN("They were the dance set, the still's set, the picture and the "
           "tune, 50 frames each") {
        REQUIRE(ending.host.spriteSets == std::vector<std::pair<int, int>>{
                                              {DANCE_SET, 0}, {STILL_SET, 0}});
        REQUIRE(ending.host.pictures == std::vector<int>{STILL_PICTURE});
        REQUIRE(ending.host.music == std::vector<int>{ENDING_TUNE});
        REQUIRE_FALSE(scene.isLoading());
      }

      THEN("_CLOSE takes the stage away, then two frames later the panel") {
        REQUIRE_FALSE(scene.isStageShown());
        REQUIRE(scene.panel() != nullptr);
        REQUIRE(ending.panelPixel(101, 10) == WAIT_WORD_COLOR);
        ending.run(2);
        REQUIRE(scene.panel() == nullptr);
        REQUIRE(scene.border() == 0x555);
      }
    }
  }

  GIVEN("No screen handed over by the boss stage") {
    Ending ending;
    ending.session.bossExit.reset();

    THEN("Starting the ending is refused") {
      REQUIRE_THROWS_AS(ending.run(1), std::logic_error);
    }
  }
}

SCENARIO("FOTO fades the still in from white and holds it for KLIKER") {
  GIVEN("The files loaded and the screens closed") {
    Ending ending;
    EndingScene &scene = ending.scene;
    ending.run(STILL_SHOWN - 2);

    THEN("The ending tune starts at full volume before the unpack") {
      REQUIRE(ending.host.musicStarts == 1);
      REQUIRE(ending.host.volumes == std::vector<int>{63});
      REQUIRE_FALSE(scene.isShown(0));
    }

    WHEN("Unpack and Screen Open 7 have taken their two frames") {
      ending.run(2);

      THEN("The picture shows all white on a black border") {
        REQUIRE(scene.isShowingStill());
        REQUIRE(scene.isShown(0));
        REQUIRE(scene.palette(0) == effects::AmigaPalette(32, 0xFFF));
        REQUIRE(scene.border() == 0x000);
        REQUIRE(scene.screen(0).pixel(100, 100) == PICTURE_COLOR);
      }

      AND_WHEN("Wait 5, Fade 4 To 7 and Wait 60 are over") {
        ending.run(5 + 60 + 2);

        THEN("The picture has its own colours and the text box is up") {
          REQUIRE(scene.palette(0) == picturePalette());
          REQUIRE(scene.bobs().isActive(1));
          REQUIRE(scene.bobs().x(1) == 40);
          REQUIRE(scene.bobs().y(1) == 80);
          REQUIRE(scene.bobs().image(1) == 1);
          REQUIRE_FALSE(scene.isShown(1));
        }
      }
    }
  }

  GIVEN("The text box over the still") {
    Ending ending;
    EndingScene &scene = ending.scene;
    ending.run(KLIKER_START);

    WHEN("Nothing is touched") {
      ending.run(1999);
      const bool held = scene.bobs().isActive(1);
      ending.run(1 + 14);
      const bool stillHeld = scene.bobs().isActive(1);
      ending.run(1);

      THEN("KLIKER waits 2000 frames, then Wait 15 and _OFF") {
        REQUIRE(held);
        REQUIRE(stillHeld);
        REQUIRE_FALSE(scene.bobs().isActive(1));
      }
    }

    WHEN("The joystick is pushed") {
      ending.run(100);
      ending.run(1, JOY_RIGHT);
      ending.run(14);
      const bool held = scene.bobs().isActive(1);
      ending.run(1);

      THEN("The wait ends at once, and the box goes 15 frames later") {
        REQUIRE(held);
        REQUIRE_FALSE(scene.bobs().isActive(1));
        REQUIRE(scene.isShowingStill());
      }
    }
  }
}

SCENARIO("Franko walks away into the light, one image per 22 frames") {
  GIVEN("The text box dismissed") {
    Ending ending;
    EndingScene &scene = ending.scene;
    ending.run(KLIKER_START);
    ending.run(1, JOY_FIRE);
    ending.run(15 + 19);

    WHEN("Cls 0 and Fade 5 To 1 have had their Wait 50") {
      const uint8_t picture = scene.screen(0).pixel(100, 100);
      ending.run(1);
      const effects::AmigaPalette grey = scene.palette(0);
      ending.run(50);

      THEN("The still is cleared and faded to grey, and BACK[-1] greys the "
           "border") {
        REQUIRE(picture == PICTURE_COLOR);
        REQUIRE(grey[0] == 0x000);
        REQUIRE(scene.palette(0)[0] == 0x444);
        REQUIRE(scene.border() == 0x444);
        REQUIRE(scene.isWalkingAway());
      }

      THEN("The doorway and the farewell text are pasted, Franko stands in "
           "front") {
        REQUIRE(scene.screen(0).pixel(80, 128) == STILL_MARK + 25);
        REQUIRE(scene.screen(0).pixel(40, 16) == STILL_MARK + 2);
        REQUIRE(scene.screen(0).pixel(100, 100) == 0);
        REQUIRE(scene.bobs().x(2) == 142);
        REQUIRE(scene.bobs().y(2) == 148);
        REQUIRE(scene.bobs().image(2) == 3);
      }

      AND_WHEN("The walk plays out") {
        std::vector<std::pair<int, int>> images;
        int frame = 0;
        while (scene.isWalkingAway() && frame < 3000) {
          const int image = scene.bobs().image(2);
          ending.run(1);
          ++frame;
          if (scene.bobs().image(2) != image) {
            images.emplace_back(frame, scene.bobs().image(2));
          }
          if (scene.machine().isRunning(2) == false) {
            break;
          }
        }

        THEN("Images 4 to 24 follow every 22 frames, sinking a pixel "
             "each, then 26") {
          REQUIRE(images.size() == 22);
          for (std::size_t i = 0; i + 1 < images.size(); ++i) {
            REQUIRE(images[i].second == static_cast<int>(i) + 4);
            REQUIRE(images[i].first == 23 + 22 * static_cast<int>(i));
          }
          REQUIRE(images.back() == std::pair<int, int>{484, 26});
          REQUIRE(scene.bobs().y(2) == 148 + 22);
        }
      }
    }
  }
}

SCENARIO("The break-dance opens two screens and walks the dancer in") {
  GIVEN("The farewell dismissed") {
    Ending ending;
    EndingScene &scene = ending.scene;
    ending.reachLastWalkFrame();
    ending.run(1, JOY_FIRE);

    THEN("Cls 0 and Fade 5 take the still to black under the grey border") {
      ending.run(49);
      REQUIRE(scene.isWalkingAway());
      REQUIRE(scene.palette(0)[0] == 0x000);
      REQUIRE(scene.border() == 0x444);
      REQUIRE(scene.screen(0).pixel(80, 128) == 0);
    }

    WHEN("_OFF, _CLOSE and the dancer's screen have taken their frames") {
      ending.run(50 + 2 + 2);
      const bool closed = !scene.isShown(0) && !scene.isShown(1);
      ending.run(1);

      THEN("Screen 1 opens at line 131 with the orange ramp") {
        REQUIRE(closed);
        REQUIRE(scene.isShown(1));
        REQUIRE(scene.screen(1).height() == 164);
        REQUIRE(scene.palette(1)[1] == 0x06F);
        REQUIRE(scene.palette(1)[15] == 0xFFF);
        REQUIRE(scene.border() == 0x444);
        std::vector<uint32_t> frame;
        scene.compose(frame);
        REQUIRE(frame[80 * 320] == 0xFF444444u);
        REQUIRE(frame[81 * 320] == 0xFF000000u);
      }

      AND_WHEN("Double Buffer is over") {
        ending.run(3);

        THEN("The dancer waits mirrored at 380,163 and the portraits at "
             "400,15") {
          REQUIRE(scene.bobs().x(1) == 380);
          REQUIRE(scene.bobs().y(1) == 163);
          REQUIRE(static_cast<uint16_t>(scene.bobs().image(1)) == 0x8006);
          for (int bob = 2; bob <= 4; ++bob) {
            REQUIRE(scene.bobs().x(bob) == 400);
            REQUIRE(scene.bobs().y(bob) == 15);
            REQUIRE(scene.bobs().image(bob) == bob - 1);
          }
          REQUIRE_FALSE(scene.isShown(0));
        }

        AND_WHEN("Screen Open 0 has taken its frame") {
          ending.run(1);

          THEN("The text screen sits at line 50 in black on a black border") {
            REQUIRE(scene.isShown(0));
            REQUIRE(scene.screen(0).height() == 80);
            REQUIRE(scene.palette(0) == effects::AmigaPalette(16, 0x000));
            REQUIRE(scene.border() == 0x000);
            REQUIRE(scene.isShowingCredits());
            REQUIRE(scene.page() == 0);
          }

          THEN("The actors have had their first frame") {
            REQUIRE(scene.bobs().x(1) == 379);
            REQUIRE(static_cast<uint16_t>(scene.bobs().image(1)) == 0x8004);
            REQUIRE(scene.bobs().x(2) == 400);
          }

          AND_WHEN("320 frames have passed") {
            ending.run(320);

            THEN("The dancer has walked in to x 60 and the portraits have "
                 "not moved") {
              REQUIRE(scene.bobs().x(1) == 60);
              REQUIRE(scene.bobs().x(2) == 400);
            }
          }

          AND_WHEN("The first portrait's 800-frame wait and 580-frame walk "
                   "are over") {
            ending.run(800 + 580);

            THEN("AMAL's 8.8 steps leave it at x 92, not 90, and the next "
                 "one still waits") {
              REQUIRE(scene.bobs().x(2) == 92);
              REQUIRE(scene.bobs().x(3) == 400);
            }
          }
        }
      }
    }
  }
}

SCENARIO("Each credit page is pasted in glyphs and flashed by BLYSK2") {
  GIVEN("The first page") {
    Ending ending;
    EndingScene &scene = ending.scene;
    ending.reachCredits();
    const EndingCredits &credits = ending.host.credits;
    const std::string &text = credits.pages[0].lines[0].text;
    const int x = (280 - static_cast<int>(text.size()) * 16) / 2;

    THEN("Glyph images are the stored codes plus 6, centred in 280 px") {
      REQUIRE(ending.host.creditLoads == 1);
      REQUIRE(credits.pages[0].lines[0].y == 16);
      for (std::size_t i = 0; i < text.size(); ++i) {
        const int glyphX = 16 * (static_cast<int>(i) + 1) + x;
        REQUIRE(scene.screen(0).pixel(glyphX, 16) ==
                static_cast<unsigned char>(text[i]) + 6);
      }
      REQUIRE(scene.screen(0).pixel(x + 15, 16) == 0);
    }

    THEN("Colours 1 and 2 fade to white and grey in 141 frames, colour 0 "
         "stays black") {
      ending.run(140);
      REQUIRE(scene.palette(0)[1] != 0xFFF);
      ending.run(1);
      REQUIRE(scene.palette(0)[0] == 0x000);
      REQUIRE(scene.palette(0)[1] == 0xFFF);
      REQUIRE(scene.palette(0)[2] == 0xAAA);
    }

    THEN("The page holds 150 frames, fades out for 150, then Cls 0 and "
         "the next page") {
      ending.run(149);
      REQUIRE(scene.palette(0)[1] == 0xFFF);
      ending.run(1 + 141);
      REQUIRE(scene.palette(0)[1] == 0x000);
      REQUIRE(scene.screen(0).pixel(x + 16, 16) ==
              static_cast<unsigned char>(text[0]) + 6);
      ending.run(8);
      REQUIRE(scene.page() == 0);
      ending.run(1);
      REQUIRE(scene.page() == 1);
      REQUIRE(scene.screen(0).pixel(x + 16, 16) == 0);
      const std::string &next = credits.pages[1].lines[0].text;
      const int nextX = (280 - static_cast<int>(next.size()) * 16) / 2;
      REQUIRE(scene.screen(0).pixel(nextX + 16, 16) ==
              static_cast<unsigned char>(next[0]) + 6);
    }
  }

  GIVEN("The credits running") {
    Ending ending;
    EndingScene &scene = ending.scene;
    ending.reachCredits();

    WHEN("The tenth page is reached") {
      const int frames =
          ending.runUntil([&] { return scene.page() == 10; }, 20000);
      const int16_t dancer = scene.bobs().image(1);
      ending.run(1);

      THEN("It follows the beats 300+I apart and starts the second dance") {
        REQUIRE(frames == beatsBefore(ending.host.credits, 10));
        REQUIRE(dancer != 27);
        REQUIRE(scene.bobs().image(1) == 27);
        REQUIRE(scene.bobs().x(2) == 400 - 1);
        REQUIRE(scene.bobs().x(3) == 550 - 1);
        REQUIRE(scene.bobs().x(4) == 700 - 1);
      }
    }
  }
}

SCENARIO("After the last page the music fades out and ETAP goes to HI") {
  GIVEN("The credits running") {
    Ending ending;
    EndingScene &scene = ending.scene;
    ending.reachCredits();
    const int pages =
        ending.runUntil([&] { return !scene.isShowingCredits(); }, 20000);

    THEN("Every page takes 300 frames plus its beat") {
      REQUIRE(pages == beatsBefore(ending.host.credits, CREDIT_PAGES));
      REQUIRE(scene.page() == CREDIT_PAGES);
      REQUIRE(scene.isShown(0));
    }

    WHEN("Fire ends the final KLIKER") {
      ending.host.volumes.clear();
      ending.run(1, JOY_FIRE);
      const bool textClosed = !scene.isShown(0) && scene.isShown(1);
      ending.run(2);
      const bool allClosed = !scene.isShown(0) && !scene.isShown(1);
      const int ended =
          ending.runUntil([&] { return scene.isFinished(); }, 200);

      THEN("_CLOSE shuts both screens, SCICH steps the volume down 64 "
           "frames, then HI gets ETAP") {
        REQUIRE(textClosed);
        REQUIRE(allClosed);
        REQUIRE(ended == 1 + 64 + 1);
        std::vector<int> expected;
        for (int volume = 63; volume >= 0; --volume) {
          expected.push_back(volume);
        }
        expected.push_back(63);
        REQUIRE(ending.host.volumes == expected);
        REQUIRE(ending.host.musicStops == 2);
        REQUIRE(ending.session.stageReached == 3);
        REQUIRE_FALSE(scene.bobs().isActive(1));
        REQUIRE_FALSE(scene.machine().exists(1));
      }
    }
  }
}

SCENARIO("The credits come from the JSON that frankoExtract writes") {
  GIVEN("Two pages as the extractor lays them out") {
    const std::string json =
        "{\"pages\": [{\"beat\": 0, \"lines\": [{\"y\": 16, \"text\": "
        "\"AB\\\\C\"}]}, {\"beat\": 150, \"lines\": [{\"y\": 0, \"text\": "
        "\"D%E\"}, {\"y\": 32, \"text\": \"F\"}]}]}";
    const EndingCredits credits = EndingCredits::fromJson(json);

    THEN("Each page keeps its lines, heights and beat") {
      REQUIRE(credits.pages.size() == 2);
      REQUIRE(credits.pages[0].beat == 0);
      REQUIRE(credits.pages[0].lines[0].text == "AB\\C");
      REQUIRE(credits.pages[0].lines[0].y == 16);
      REQUIRE(credits.pages[1].beat == 150);
      REQUIRE(credits.pages[1].lines.size() == 2);
      REQUIRE(credits.pages[1].lines[1].text == "F");
      REQUIRE(credits.pages[1].lines[1].y == 32);
    }
  }

  GIVEN("Malformed credits") {
    THEN("They are refused") {
      REQUIRE_THROWS_AS(EndingCredits::fromJson("{}"), std::invalid_argument);
      REQUIRE_THROWS_AS(
          EndingCredits::fromJson(
              "{\"pages\": [{\"beat\": 0, \"lines\": [{\"y\": 1}]}]}"),
          std::invalid_argument);
      REQUIRE_THROWS_AS(
          EndingCredits::fromJson("{\"pages\": [{\"beat\": 0, \"lines\": "
                                  "[{\"y\": 1, \"text\": 5}]}]}"),
          std::invalid_argument);
    }
  }

  GIVEN("An ending whose host has no credits") {
    Ending ending;
    ending.host.credits = EndingCredits{};
    ending.reachCredits();

    THEN("The text screen opens and goes straight to the last KLIKER") {
      REQUIRE(ending.scene.isShown(0));
      REQUIRE_FALSE(ending.scene.isShowingCredits());
      REQUIRE(ending.scene.page() == 0);
    }
  }
}
