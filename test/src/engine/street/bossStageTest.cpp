#include "../../../../src/engine/street/BossStage.h"
#include "../../../../src/engine/street/StageFrame.h"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <cstdlib>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street;

namespace {

constexpr int RB = 1;
constexpr int RD = 3;
constexpr int RE = 4;
constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RI = 8;
constexpr int RN = 13;
constexpr int RO = 14;
constexpr int RQ = 16;
constexpr int RR = 17;
constexpr int RS = 18;
constexpr int RT = 19;
constexpr int RU = 20;
constexpr int RW = 22;
constexpr int RX = 23;

constexpr int16_t JOY_LEFT = 4;
constexpr int16_t JOY_RIGHT = 8;
constexpr int16_t JOY_FIRE = 16;

constexpr int FRANKO_BOSS_SET = 0xFE;
constexpr int FRANKO_EXTRA_PHASES = 0xFD;
constexpr int STAGE_1_BOSS = 0xC8;
constexpr int STAGE_3_BOSS = 0xC6;
constexpr int EXIT_X = 164;
constexpr uint8_t STREET_COLOR = 6;
constexpr uint8_t STAMP_COLOR = 3;
constexpr uint8_t PLAYER_COLOR = 1;
constexpr uint8_t BOSS_COLOR = 2;
constexpr uint8_t STRIP_COLOR = 7;
constexpr uint8_t WAIT_WORD_COLOR = 5;
constexpr int BOSS_FILES = 6;
constexpr int READY_FRAMES = 1 + BOSS_FILES * LoadingMock::FILE_FRAMES;

Picture box(int width, int height, int hotX, int hotY, uint8_t color) {
  return Picture{
      width, height, hotX, hotY,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height), color)};
}

uint8_t columnColor(int column) { return static_cast<uint8_t>(100 + column); }

class FakeHost : public StreetHost {
public:
  struct Sample {
    int bank;
    int sample;
    int voices;
    bool operator==(const Sample &other) const {
      return bank == other.bank && sample == other.sample &&
             voices == other.voices;
    }
  };

  std::vector<std::pair<int, int>> spriteSets;
  std::vector<int> scenery;
  std::vector<int> music;
  int musicStarts = 0;
  int musicStops = 0;
  std::vector<int> volumes;
  std::vector<Sample> samples;
  std::vector<bool> loops;
  std::function<int(int)> randomValue = [](int limit) { return limit; };

  std::vector<Picture> loadSpriteSet(int resource, int sampleBank) override {
    spriteSets.emplace_back(resource, sampleBank);
    std::vector<Picture> frames;
    if (resource == 0) {
      frames.push_back(box(48, 23, 0, 11, 5));
      for (int i = 1; i < 9; ++i) {
        frames.push_back(box(16, 8, 0, 0, 3));
      }
      frames.push_back(box(16, 1, 0, 0, 0));
      frames.push_back(box(16, 1, 0, 0, 0));
    } else if (resource == FRANKO_BOSS_SET) {
      for (int i = 0; i < 32; ++i) {
        frames.push_back(box(64, 80, 16, 77, PLAYER_COLOR));
      }
    } else if (resource >= FRANKO_BOSS_SET - 3 && resource < FRANKO_BOSS_SET) {
      for (int i = 0; i < 2; ++i) {
        frames.push_back(box(64, 80, 16, 77, PLAYER_COLOR));
      }
    } else if (resource == STAGE_3_BOSS) {
      for (int i = 0; i < 52; ++i) {
        Picture frame = box(64, 80, 16, 77, BOSS_COLOR);
        frame.pixels[0] = 0;
        frame.pixels[1] = static_cast<uint8_t>(43 + i);
        frames.push_back(frame);
      }
    } else {
      for (int i = 0; i < 52; ++i) {
        frames.push_back(box(64, 80, 16, 77, BOSS_COLOR));
      }
    }
    return frames;
  }

  Picture loadPicture(int) override { return box(320, 222, 0, 0, 0); }

  effects::AmigaPalette loadPalette(int) override { return {}; }

  std::vector<Picture> loadScenery(int resource) override {
    scenery.push_back(resource);
    std::vector<Picture> columns;
    for (int i = 0; i < 19; ++i) {
      columns.push_back(box(16, 222, 0, 0, columnColor(i)));
    }
    return columns;
  }

  LevelScript loadLevelScript(int) override { return LevelScript{}; }

  EndingCredits loadEndingCredits() override { return {}; }

  Picture loadPanelPicture(int part) override {
    if (part != 0) {
      return box(304, 40, 0, 0, 1);
    }
    Picture strip = box(304, 48, 0, 0, STRIP_COLOR);
    std::fill(strip.pixels.begin() + 32 * 304, strip.pixels.end(),
              WAIT_WORD_COLOR);
    return strip;
  }

  void loadMusic(int resource) override { music.push_back(resource); }

  bool isMusicLoaded(int) const override { return false; }

  void playMusic() override { ++musicStarts; }

  void stopMusic() override { ++musicStops; }

  void setMusicVolume(int volume) override { volumes.push_back(volume); }
  void setMusicTempo(int) override {}

  void playSample(int bank, int sample, int voices) override {
    samples.push_back({bank, sample, voices});
  }

  void playSampleAt(int, int, int, int) override {}

  void setSampleLoop(bool loop) override { loops.push_back(loop); }

  int random(int limit) override { return randomValue(limit); }

  bool played(int bank, int sample, int voices) const {
    return std::find(samples.begin(), samples.end(),
                     Sample{bank, sample, voices}) != samples.end();
  }
};

struct Duel {
  FakeHost host;
  GameSession session;
  effects::GameOptions options;
  std::unique_ptr<BossStage> stage;

  Duel() {
    session.registers[RO] = 1;
    session.registers[RF] = 64;
    session.registers[RG] = 3;
    session.registers[RB] = 172;
    IndexedSurface screen(320, 222);
    screen.fill(STREET_COLOR);
    const ScreenBlock block(screen, EXIT_X - 16, 172 - 77, 48, 78);
    screen.clear(STAMP_COLOR, EXIT_X - 16, 172 - 77, EXIT_X + 16, 172 + 2);
    session.streetExit.emplace(
        StreetExit{screen, block, EXIT_X, 64, 0, std::nullopt});
  }

  BossStage &start() {
    stage = std::make_unique<BossStage>(host, session, options);
    return *stage;
  }

  void run(int frames, int16_t joystick = 0, SystemKey key = SystemKey::None) {
    for (int frame = 0; frame < frames; ++frame) {
      stage->advance({joystick, frame == 0 ? key : SystemKey::None});
    }
  }

  int runUntil(const std::function<bool()> &done, int limit,
               int16_t joystick = 0) {
    for (int frame = 0; frame < limit; ++frame) {
      stage->advance({joystick, SystemKey::None});
      if (done()) {
        return frame + 1;
      }
    }
    return -1;
  }

  uint8_t panelPixel(int x, int y) const {
    return stage->panel()->surface().pixel(x, y);
  }

  int reachDialogue(int16_t walk = JOY_RIGHT) {
    return runUntil([this] { return stage->isTalking(); }, 1000, walk);
  }

  int reachFight(int16_t walk = JOY_RIGHT) {
    reachDialogue(walk);
    return runUntil([this] { return stage->isFighting(); }, 1000, JOY_FIRE);
  }

  int16_t &global(int index) { return session.registers[index]; }
};

} // namespace

SCENARIO("The boss stage reloads the cast over the street's last screen") {
  GIVEN("Franko at the end of stage 1 with the music on") {
    Duel duel;
    BossStage &stage = duel.start();
    duel.run(1);

    THEN("ERA stops the street's tune and the boss tune's file is read") {
      REQUIRE(duel.host.musicStops == 1);
      REQUIRE(duel.host.music == std::vector<int>{604});
      REQUIRE(duel.host.musicStarts == 0);
      REQUIRE(duel.panelPixel(101, 10) == STRIP_COLOR);
      REQUIRE_FALSE(duel.session.streetExit.has_value());
    }

    THEN("The street's last screen stays up with the player stamped in it") {
      REQUIRE(stage.screen().pixel(0, 0) == STREET_COLOR);
      REQUIRE(stage.screen().pixel(EXIT_X, 150) == STAMP_COLOR);
      REQUIRE_FALSE(stage.bobs().isActive(1));
    }

    WHEN("The tune's file has been read and unpacked") {
      duel.run(LoadingMock::READ_FRAMES);
      const uint8_t unpacking = duel.panelPixel(101, 10);
      duel.run(LoadingMock::UNPACK_FRAMES);

      THEN("CZEKAJ showed the wait word, then MUZON started the tune") {
        REQUIRE(unpacking == WAIT_WORD_COLOR);
        REQUIRE(duel.host.musicStarts == 1);
        REQUIRE(duel.host.volumes.front() == 30);
        REQUIRE(duel.host.scenery == std::vector<int>{320});
      }
    }

    WHEN("All six files are in") {
      const int ready =
          duel.runUntil([&] { return stage.isApproaching(); }, 1000) + 1;

      THEN("They were the approach columns, the blood and the three sets") {
        REQUIRE(ready == READY_FRAMES);
        REQUIRE(duel.host.spriteSets ==
                std::vector<std::pair<int, int>>{{0, 0},
                                                 {FRANKO_BOSS_SET, 2},
                                                 {FRANKO_EXTRA_PHASES, 0},
                                                 {STAGE_1_BOSS, 4}});
      }

      THEN("Put Block takes the stamp back and the player is a bob again") {
        REQUIRE(stage.screen().pixel(EXIT_X, 150) == STREET_COLOR);
        REQUIRE(stage.bobs().x(1) == EXIT_X);
        REQUIRE(stage.bobs().y(1) == 172);
        REQUIRE(stage.bobs().image(1) == 17);
      }

      THEN("Every actor of the duel runs on its channel") {
        for (int channel : {0, 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 15}) {
          REQUIRE(stage.machine().exists(channel));
        }
        REQUIRE(stage.machine().channelRegister(1, 2) == 164);
        REQUIRE(stage.machine().channelRegister(3, 0) == 272);
        REQUIRE(duel.global(RI) == 1);
      }

      AND_WHEN("The actors have had their first frame") {
        duel.run(1);

        THEN("The boss waits off-screen and the spectator is hidden") {
          REQUIRE(stage.bobs().x(2) == 470);
          REQUIRE(stage.bobs().y(2) == 108);
          REQUIRE(stage.bobs().image(2) == 78);
          REQUIRE(stage.bobs().x(3) == 176);
          REQUIRE(stage.bobs().image(3) == 10);
        }
      }
    }
  }

  GIVEN("No screen handed over by the street") {
    Duel duel;
    duel.session.streetExit.reset();
    duel.start();

    THEN("Starting the stage is refused") {
      REQUIRE_THROWS_AS(duel.run(1), std::logic_error);
    }
  }
}

SCENARIO("The boss stage keeps the display lines state 09 and SYS set") {
  GIVEN("NTSC chosen") {
    Duel duel;
    duel.options.ntsc = true;
    BossStage &stage = duel.start();
    duel.run(1);
    std::vector<uint32_t> frame;
    stage.compose(frame);

    THEN("The play screen sits on line 7, its top rows above line 26 lost") {
      const effects::AmigaPalette &colors = levelPalette(false);
      REQUIRE(frame.size() == 304u * 255u);
      REQUIRE(frame[18 * 304] == 0xFF000000u);
      REQUIRE(frame[19 * 304] == toArgb(colors[STREET_COLOR]));
    }
  }

  GIVEN("A PAL approach under way") {
    Duel duel;
    BossStage &stage = duel.start();
    duel.run(READY_FRAMES + 20);

    WHEN("F4 is pressed") {
      duel.run(1, 0, SystemKey::Ntsc);
      for (int frame = 0; frame < 20 && !duel.options.ntsc; ++frame) {
        duel.run(1);
      }
      std::vector<uint32_t> sysFrame;
      stage.compose(sysFrame);
      duel.run(1);
      std::vector<uint32_t> beamFrame;
      stage.compose(beamFrame);
      duel.run(1);
      std::vector<uint32_t> frame;
      stage.compose(frame);

      THEN("SYS's frame is PAL, the next has the NTSC beam over the PAL "
           "lines, and a VBL later the screens are on their NTSC lines") {
        REQUIRE(duel.options.ntsc);
        REQUIRE(sysFrame[18 * 304] != 0xFF000000u);
        REQUIRE(beamFrame[18 * 304] == 0xFF000000u);
        REQUIRE(beamFrame[19 * 304] == 0xFF555555u);
        REQUIRE(beamFrame[40 * 304] == sysFrame[0]);
        REQUIRE(frame[18 * 304] == 0xFF000000u);
        REQUIRE(frame[19 * 304] == sysFrame[19 * 304]);
      }
    }
  }

  GIVEN("320x512 chosen") {
    Duel duel;
    duel.options.tallScreen = true;
    BossStage &stage = duel.start();
    duel.run(1);
    std::vector<uint32_t> frame;
    stage.compose(frame);

    THEN("The laced play screen starts 60 lines lower in a double-height "
         "frame") {
      const effects::AmigaPalette &colors = levelPalette(false);
      REQUIRE(frame.size() == 304u * 510u);
      REQUIRE(frame[119 * 304] == 0xFF555555u);
      REQUIRE(frame[120 * 304] == toArgb(colors[STREET_COLOR]));
    }
  }
}

SCENARIO("Walking to the boss scrolls nineteen columns as state 14 does") {
  GIVEN("The approach") {
    Duel duel;
    BossStage &stage = duel.start();
    duel.run(READY_FRAMES + 1);

    WHEN("Right is held until the conversation starts") {
      std::vector<int> columnFrames;
      int column = stage.columnsWalked();
      int frame = 0;
      while (!stage.isTalking() && frame < 1000) {
        duel.run(1, JOY_RIGHT);
        ++frame;
        if (stage.columnsWalked() != column) {
          column = stage.columnsWalked();
          columnFrames.push_back(frame);
        }
      }

      THEN("Each 16 px column takes 9 frames and there are 19 of them") {
        REQUIRE(stage.columnsWalked() == 19);
        REQUIRE(columnFrames.size() == 19);
        for (std::size_t i = 1; i < columnFrames.size(); ++i) {
          REQUIRE(columnFrames[i] - columnFrames[i - 1] == 9);
        }
      }

      THEN("The boss ratchets 8 px per scroll and the clamp 4 px") {
        REQUIRE(stage.bobs().x(2) == 470 - 37 * 8);
        REQUIRE(stage.machine().channelRegister(3, 0) == 272 - 37 * 4);
      }

      THEN("The last columns sit at the right edge of the screen") {
        REQUIRE(stage.screen().pixel(296, 100) == columnColor(18));
        REQUIRE(stage.screen().pixel(280, 100) == columnColor(17));
      }

      THEN("The boss opens the conversation while both walkers freeze") {
        REQUIRE(duel.global(RX) == 2);
        REQUIRE(duel.global(RT) == 1);
        REQUIRE(stage.bobs().x(5) == 176);
        REQUIRE(stage.bobs().y(5) == 40);
        REQUIRE(stage.bobs().image(5) == 91);
        REQUIRE(stage.machine().isFrozen(1));
        REQUIRE(stage.machine().isFrozen(4));
        REQUIRE_FALSE(stage.machine().exists(13));
        REQUIRE(stage.bobs().image(12) == 10);
      }
    }

    WHEN("Escape is pressed on the way") {
      duel.run(10, JOY_RIGHT, SystemKey::Escape);

      THEN("The run quits with the score cleared") {
        REQUIRE(stage.outcome() == BossStage::Outcome::Quit);
        REQUIRE(duel.global(RN) == 0);
        REQUIRE(duel.global(RO) == -1);
      }
    }
  }
}

SCENARIO("The conversation waits for the fire button as state 15 does") {
  GIVEN("The conversation has started") {
    Duel duel;
    BossStage &stage = duel.start();
    duel.run(READY_FRAMES + 1);
    duel.reachDialogue();

    WHEN("Nothing is pressed") {
      duel.run(500);

      THEN("The boss's first line stays up") {
        REQUIRE(stage.isTalking());
        REQUIRE(duel.global(RT) == 1);
        REQUIRE(stage.bobs().image(5) == 91);
      }
    }

    WHEN("Fire is held") {
      const int frames =
          duel.runUntil([&] { return stage.isFighting(); }, 1000, JOY_FIRE);

      THEN("Four lines held 50 frames each lead into the fight at 207") {
        REQUIRE(frames == 207);
        REQUIRE(stage.bobs().image(4) == 10);
      }

      THEN("The boss has 80 energy and the arena opens up to Y 124") {
        REQUIRE(stage.machine().channelRegister(5, 7) == 80);
        REQUIRE(stage.machine().channelRegister(1, 2) == 124);
        REQUIRE(stage.machine().channelRegister(3, 0) == 272);
        REQUIRE(duel.host.played(2, 9, 1));
        REQUIRE_FALSE(stage.machine().isFrozen(1));
        REQUIRE_FALSE(stage.machine().isFrozen(4));
      }
    }
  }
}

SCENARIO("The boss referee resolves hits and sounds as state 16 does") {
  GIVEN("The fight with dice that never pick a boss attack") {
    Duel duel;
    BossStage &stage = duel.start();
    duel.run(READY_FRAMES + 1);
    duel.reachFight();

    WHEN("The boss has walked up to the player") {
      const int reached = duel.runUntil(
          [&] {
            const auto &bobs = stage.bobs();
            return duel.global(RX) == 3 && bobs.y(2) == bobs.y(1) &&
                   std::abs(bobs.x(2) - bobs.x(1)) <= 32 &&
                   stage.machine().channelRegister(4, 4) == 0 &&
                   stage.machine().channelRegister(4, 5) == 0;
          },
          2000);

      AND_WHEN("The player punches") {
        duel.runUntil([&] { return duel.global(RD) != 0; }, 10, JOY_FIRE);
        duel.run(2);

        THEN("The boss's walk freezes and its damage channel takes it") {
          REQUIRE(reached > 0);
          REQUIRE(stage.machine().isFrozen(4));
          REQUIRE(stage.machine().channelRegister(5, 0) == 1);
          REQUIRE(stage.machine().channelRegister(5, 7) == 76);
          REQUIRE(duel.host.played(2, 1, 1));
        }

        AND_WHEN("The hit has played out") {
          duel.run(60);

          THEN("The boss walks again on 76 energy") {
            REQUIRE(stage.machine().channelRegister(5, 7) == 76);
            REQUIRE_FALSE(stage.machine().isFrozen(4));
          }
        }
      }
    }

    WHEN("RW asks for a boss sample and RE for a player one") {
      duel.global(RW) = 12;
      duel.global(RE) = 3;
      duel.run(1);
      duel.run(1);

      THEN("RW 12 plays sample 4 of bank 4, RE 3 sample 3 of bank 2") {
        REQUIRE(duel.host.played(4, 4, 1));
        REQUIRE(duel.host.played(2, 3, 1));
      }
    }

    WHEN("The last life is lost") {
      duel.global(RG) = -2;
      duel.run(1);
      const int ended = duel.runUntil(
          [&] { return stage.outcome() != BossStage::Outcome::Playing; }, 300);

      THEN("Game over follows state 19's Wait 200 and _CLOSE's two screens, "
           "four VBLs each") {
        REQUIRE(stage.outcome() == BossStage::Outcome::GameOver);
        REQUIRE(ended == 200 + 8);
        REQUIRE_FALSE(stage.isScreenShown());
        REQUIRE_FALSE(stage.isPanelShown());
        REQUIRE(duel.global(RO) == -1);
      }

      THEN("ETAP remembers the boss's stage") {
        REQUIRE(duel.session.stageReached == 1);
      }
    }
  }
}

SCENARIO("Beating the boss plays KONBOSS and clears the screen") {
  GIVEN("The fight without the brutality cheat") {
    Duel duel;
    BossStage &stage = duel.start();
    duel.run(READY_FRAMES + 1);
    duel.reachFight();
    const int x = stage.bobs().x(1);

    WHEN("The boss's death leaves no enemy alive") {
      duel.global(RI) = 0;
      duel.run(1);

      THEN("Only the walk-off is left: sample 4 and 340 px to the right") {
        REQUIRE(stage.isFinishing());
        REQUIRE_FALSE(stage.machine().exists(3));
        REQUIRE_FALSE(stage.machine().exists(4));
        REQUIRE(stage.machine().exists(1));
        REQUIRE(duel.global(RT) == 340);
        REQUIRE(duel.global(RU) == 340);
        REQUIRE(duel.host.played(2, 4, 1));
        REQUIRE(duel.host.loops.empty());
      }

      AND_WHEN("BASIC's Wait of 340 frames is over") {
        duel.run(340);

        THEN("The player walked 340 px and _OFF and Cls 0 blanked the screen") {
          REQUIRE(stage.bobs().x(1) == x + 340);
          const auto &pixels = stage.screen().pixels();
          REQUIRE(std::all_of(pixels.begin(), pixels.end(),
                              [](uint8_t pixel) { return pixel == 0; }));
          REQUIRE_FALSE(stage.bobs().isActive(1));
          REQUIRE_FALSE(stage.machine().exists(2));
          REQUIRE(stage.outcome() == BossStage::Outcome::Playing);
        }

        AND_WHEN("The Cls stall has passed") {
          duel.run(3);

          THEN("The stage ends where the bonus drive begins") {
            REQUIRE(stage.outcome() == BossStage::Outcome::BossDefeated);
            REQUIRE_FALSE(duel.session.bossExit.has_value());
          }
        }
      }
    }
  }

  GIVEN("The fight with the brutality cheat") {
    Duel duel;
    duel.session.brutality = true;
    BossStage &stage = duel.start();
    duel.run(READY_FRAMES + 1);
    duel.reachFight();

    WHEN("The boss dies") {
      duel.global(RI) = 0;
      duel.run(1);

      THEN("The player first walks to the body") {
        REQUIRE(stage.isFinishing());
        REQUIRE(duel.global(RS) ==
                (std::abs(duel.global(RU)) + std::abs(duel.global(RT))) / 2);
        REQUIRE_FALSE(duel.host.played(2, 4, 1));
      }

      AND_WHEN("The scene runs to its end") {
        const int ended = duel.runUntil(
            [&] { return stage.outcome() != BossStage::Outcome::Playing; },
            2000);

        THEN("The looping splash of sample 9 frames 100 stamps, then the "
             "walk-off") {
          REQUIRE(ended > 0);
          REQUIRE(stage.outcome() == BossStage::Outcome::BossDefeated);
          REQUIRE(duel.host.loops == std::vector<bool>{true, false});
          REQUIRE(duel.host.played(4, 9, 1));
          REQUIRE(duel.host.played(2, 4, 1));
        }
      }
    }
  }
}

SCENARIO("On stage 2 KONBOSS lifts the boss overhead before the walk-off") {
  GIVEN("Alex fighting the second boss") {
    Duel duel;
    duel.global(RO) = 2;
    duel.global(RQ) = 1;
    BossStage &stage = duel.start();
    duel.run(READY_FRAMES + 1);
    duel.reachFight(JOY_LEFT);
    REQUIRE(stage.isFighting());

    WHEN("The boss dies") {
      duel.global(RI) = 0;
      duel.run(1);
      const int dx = stage.bobs().x(2) - stage.bobs().x(1);
      const int dy = stage.bobs().y(2) - stage.bobs().y(1);

      THEN("The player walks the whole gap to the boss, half its length in "
           "frames") {
        REQUIRE(stage.isFinishing());
        REQUIRE(duel.global(RU) == dx);
        REQUIRE(duel.global(RT) == dy);
        REQUIRE(duel.global(RS) == (std::abs(dx) + std::abs(dy)) / 2);
        REQUIRE(stage.machine().exists(1));
        REQUIRE_FALSE(stage.machine().exists(4));
      }

      AND_WHEN("Wait RS is over") {
        duel.run(duel.global(RS));
        const uint16_t facing = static_cast<uint16_t>(duel.global(RR));

        THEN("He stands on the boss, and both get the throw scripts") {
          REQUIRE(stage.bobs().x(1) == stage.bobs().x(2));
          REQUIRE(stage.bobs().y(1) == stage.bobs().y(2));
          REQUIRE(static_cast<uint16_t>(stage.bobs().image(1)) ==
                  static_cast<uint16_t>(17 + facing));
          REQUIRE(duel.global(RT) == 2);
          REQUIRE(stage.machine().isRunning(4));
          REQUIRE(stage.machine().isRunning(1));
        }

        THEN("Sample 9 of the boss after 150 frames, 8 of the player 60 "
             "later, and the walk-off 50 after that") {
          duel.run(149);
          REQUIRE_FALSE(duel.host.played(4, 9, 1));
          duel.run(1);
          REQUIRE(duel.host.played(4, 9, 1));
          duel.run(59);
          REQUIRE_FALSE(duel.host.played(2, 8, 1));
          duel.run(1);
          REQUIRE(duel.host.played(2, 8, 1));
          REQUIRE_FALSE(duel.host.played(2, 4, 1));
          duel.run(50);
          REQUIRE(duel.host.played(2, 4, 1));
          REQUIRE(duel.global(RT) == -340);
          REQUIRE(static_cast<uint16_t>(stage.bobs().image(2)) ==
                  static_cast<uint16_t>(77 + facing));
          REQUIRE(static_cast<uint16_t>(stage.bobs().image(1)) ==
                  static_cast<uint16_t>(38 + facing));
        }
      }
    }
  }
}

SCENARIO("On stage 3 the boss taunts while Franko approaches") {
  GIVEN("Franko on the third boss's street") {
    Duel duel;
    duel.global(RO) = 3;
    BossStage &stage = duel.start();
    duel.run(READY_FRAMES);
    REQUIRE(stage.isApproaching());

    WHEN("He stands still") {
      std::vector<int> taunts;
      std::vector<int> images;
      for (int frame = 1; frame <= 100; ++frame) {
        const std::size_t before = duel.host.samples.size();
        duel.run(1);
        if (duel.host.samples.size() != before) {
          REQUIRE(duel.host.samples.back() == FakeHost::Sample{2, 7, 1});
          taunts.push_back(frame);
          images.push_back(stage.bobs().image(2));
        }
      }

      THEN("Sample 7 of bank 2 plays on each of his 78 frames, Timer>15 "
           "apart") {
        REQUIRE(taunts == std::vector<int>{1, 31, 61, 91});
        REQUIRE(images == std::vector<int>{78, 78, 78, 78});
      }
    }

    WHEN("RE asks for a sample on his first 78 frame") {
      duel.global(RE) = 5;
      duel.run(1);
      const auto first = duel.host.samples;
      duel.run(1);

      THEN("RE's sample wins that pass and the taunt comes a frame later") {
        REQUIRE(first == std::vector<FakeHost::Sample>{{2, 5, 1}});
        REQUIRE(duel.host.samples.back() == FakeHost::Sample{2, 7, 1});
      }
    }
  }

  GIVEN("Franko on the first boss's street") {
    Duel duel;
    BossStage &stage = duel.start();
    duel.run(READY_FRAMES + 100);

    THEN("That boss also shows 78, but never taunts") {
      REQUIRE(stage.isApproaching());
      REQUIRE_FALSE(duel.host.played(2, 7, 1));
    }
  }
}

SCENARIO("On stage 3 the boss beats the child before the conversation") {
  GIVEN("Franko at the last column of the third boss's street") {
    Duel duel;
    duel.global(RO) = 3;
    BossStage &stage = duel.start();
    duel.run(READY_FRAMES + 1);
    duel.reachDialogue();

    THEN("The walkers freeze, the boss stands over the stamped child") {
      REQUIRE(stage.isTalking());
      REQUIRE(stage.machine().isFrozen(1));
      REQUIRE(stage.machine().isFrozen(4));
      REQUIRE_FALSE(stage.machine().exists(13));
      REQUIRE(stage.bobs().image(1) == 17);
      REQUIRE(stage.bobs().image(2) == 71);
      REQUIRE(stage.screen().pixel(233, 180) == 80);
      REQUIRE(stage.screen().pixel(232, 180) != 0);
      REQUIRE(duel.global(RT) == 0);
    }

    WHEN("The scene plays out") {
      std::vector<std::pair<int, int>> poses;
      int shout = 0;
      int talk = 0;
      for (int frame = 1; frame <= 200 && talk == 0; ++frame) {
        const int image = stage.bobs().image(2);
        duel.run(1);
        if (stage.bobs().image(2) != image) {
          poses.emplace_back(frame, stage.bobs().image(2));
        }
        if (shout == 0 && duel.host.played(4, 7, 1)) {
          shout = frame;
        }
        if (duel.global(RT) != 0) {
          talk = frame;
        }
      }

      THEN("Three punches of 15 frames after the paste's 3, then the flex "
           "and sample 7 of bank 4") {
        REQUIRE(poses == std::vector<std::pair<int, int>>{{18, 81},
                                                          {33, 71},
                                                          {48, 81},
                                                          {63, 71},
                                                          {78, 81},
                                                          {93, 74},
                                                          {108, 81}});
        REQUIRE(shout == 93);
      }

      THEN("The talk starts 15 frames after the shout, without a bubble "
           "from BASIC") {
        REQUIRE(talk == 108);
        REQUIRE(duel.global(RT) == 1);
        REQUIRE(stage.isTalking());
        REQUIRE(stage.bobs().image(5) == 10);
      }

      AND_WHEN("Fire is held through the conversation") {
        const int frames =
            duel.runUntil([&] { return stage.isFighting(); }, 1000, JOY_FIRE);

        THEN("The fight starts with the shout from bank 2") {
          REQUIRE(frames > 0);
          REQUIRE(duel.host.played(2, 9, 1));
          REQUIRE(stage.machine().channelRegister(5, 7) == 80);
        }
      }
    }
  }
}

SCENARIO("On stage 3 KONBOSS sends the boss over the railing") {
  GIVEN("Franko fighting the third boss") {
    Duel duel;
    duel.global(RO) = 3;
    BossStage &stage = duel.start();
    duel.run(READY_FRAMES + 1);
    duel.reachFight();
    REQUIRE(stage.isFighting());
    const int playerX = stage.bobs().x(1);

    WHEN("The boss dies") {
      duel.global(RI) = 0;
      duel.run(1);
      const int x = stage.bobs().x(2);
      const int y = stage.bobs().y(2);
      duel.run(1);

      THEN("He rests on 75 and RU, RS, RT measure his way to the railing") {
        REQUIRE(stage.isAtRailing());
        REQUIRE(stage.isFinishing());
        REQUIRE(stage.bobs().image(2) == 75);
        REQUIRE(duel.global(RU) == 208 - x);
        REQUIRE(duel.global(RS) == 143 - y);
        REQUIRE(duel.global(RT) == (std::abs(208 - x) + std::abs(143 - y)) / 2);
        REQUIRE(duel.global(RR) == 0);
        REQUIRE(stage.bobs().image(1) == 17);
        REQUIRE_FALSE(stage.machine().exists(1));
        REQUIRE_FALSE(stage.machine().exists(3));
      }

      AND_WHEN("Wait 50 is over") {
        duel.run(48);
        const bool before =
            stage.bobs().isActive(5) && stage.bobs().image(5) == 93;
        duel.run(1);

        THEN("His bubble 93 shows above him") {
          REQUIRE_FALSE(before);
          REQUIRE(stage.bobs().x(5) == x - 32);
          REQUIRE(stage.bobs().y(5) == y - 72);
          REQUIRE(stage.bobs().image(5) == 93);
        }

        THEN("It stays up while fire is not pressed alone") {
          duel.run(200, JOY_FIRE | JOY_RIGHT);
          REQUIRE(stage.bobs().image(5) == 93);
          REQUIRE(stage.bobs().x(2) == x);
          REQUIRE(stage.bobs().image(2) == 75);
        }

        AND_WHEN("Fire is pressed") {
          const int hidden = duel.runUntil(
              [&] { return stage.bobs().image(5) == 10; }, 10, JOY_FIRE);
          const int walk = duel.global(RT);

          THEN("The boss sets off on the tick after the bubble goes") {
            REQUIRE(hidden == 2);
            REQUIRE(stage.machine().channelRegister(4, 0) == 1);
            duel.run(1);
            REQUIRE(stage.bobs().image(2) == 43);
          }

          AND_WHEN("Wait RT is over") {
            duel.run(walk - 1);
            const bool shown = stage.bobs().isActive(2);
            duel.run(1);

            THEN("He reached the railing, and Bob Off takes him away") {
              REQUIRE(shown);
              REQUIRE(stage.bobs().x(2) == 208);
              REQUIRE(stage.bobs().y(2) == 143);
              REQUIRE_FALSE(stage.bobs().isActive(2));
              REQUIRE_FALSE(stage.machine().exists(4));
            }

            AND_WHEN("A frame later he is pasted onto the railing") {
              const uint8_t wall = stage.screen().pixel(183, 60);
              duel.run(1);

              THEN("No Mask makes the stamp opaque") {
                REQUIRE(wall != 0);
                REQUIRE(stage.screen().pixel(183, 60) == 0);
                REQUIRE(stage.screen().pixel(184, 60) == 82);
              }

              THEN("After the stall and Wait 100 he curses, then falls "
                   "through 83 to 85 every 19 frames") {
                duel.run(102);
                REQUIRE_FALSE(duel.host.played(4, 9, 1));
                duel.run(1);
                REQUIRE(duel.host.played(4, 9, 1));
                REQUIRE(stage.bobs().x(5) == 220);
                REQUIRE(stage.bobs().y(5) == 64);
                REQUIRE(stage.bobs().image(5) == 94);
                REQUIRE(stage.screen().pixel(184, 60) == 82);
                duel.run(1);
                REQUIRE(stage.screen().pixel(184, 60) == 83);
                duel.run(18);
                REQUIRE(stage.screen().pixel(184, 60) == 83);
                duel.run(1);
                REQUIRE(stage.screen().pixel(184, 60) == 84);
                duel.run(19);
                REQUIRE(stage.screen().pixel(184, 60) == 85);
                REQUIRE(stage.screen().pixel(183, 60) == 0);
              }

              THEN("The bubble goes 43 frames after the last fall, and "
                   "Franko poses 90 frames later") {
                duel.run(103 + 1 + 19 + 19);
                REQUIRE(stage.screen().pixel(184, 60) == 85);
                duel.run(42);
                REQUIRE(stage.bobs().image(5) == 94);
                duel.run(1);
                REQUIRE(stage.bobs().image(5) == 10);
                REQUIRE(stage.bobs().image(1) == 17);
                duel.run(89);
                REQUIRE(stage.bobs().image(1) == 17);
                duel.run(1);
                REQUIRE(stage.bobs().image(1) == 38);
                duel.run(30);
                REQUIRE(stage.bobs().image(1) == 39);
                REQUIRE(duel.host.played(2, 3, 1));
                duel.run(40);
                REQUIRE(stage.bobs().image(1) == 38);
                REQUIRE_FALSE(duel.host.played(2, 4, 1));
                duel.run(30);
                REQUIRE(duel.host.played(2, 4, 1));
                REQUIRE(duel.global(RT) == 340);
              }

              THEN("The walk-off ends the stage; CONGRA's _OFF runs in the "
                   "same frame and its Cls goes to the hand-over") {
                const int ended = duel.runUntil(
                    [&] {
                      return stage.outcome() != BossStage::Outcome::Playing;
                    },
                    2000);
                REQUIRE(ended == 3 + 100 + 1 + 19 + 19 + 18 + 25 + 90 + 30 +
                                     40 + 30 + 340);
                REQUIRE(stage.outcome() == BossStage::Outcome::BossDefeated);
                REQUIRE(stage.bobs().x(1) == playerX + 340);
                REQUIRE(stage.screen().pixel(184, 60) == 85);
                REQUIRE_FALSE(stage.bobs().isActive(1));
                REQUIRE_FALSE(stage.machine().exists(1));
              }

              THEN("CONGRA gets the last frame as shown, bobs and panel "
                   "included") {
                duel.runUntil(
                    [&] {
                      return stage.outcome() != BossStage::Outcome::Playing;
                    },
                    2000);
                REQUIRE(duel.session.bossExit.has_value());
                BossExit exit = *duel.session.bossExit;
                REQUIRE(exit.buffer.isAutobacking());
                exit.buffer.vbl();
                REQUIRE(exit.buffer.shown().pixels() !=
                        stage.screen().pixels());
                REQUIRE(exit.palette == levelPalette(false));
                REQUIRE(exit.displayY == 47);
                REQUIRE(exit.offsetX == 0);
                REQUIRE(exit.panelY == 270);
                REQUIRE_FALSE(exit.laced);
                REQUIRE(exit.panel.pixels() ==
                        stage.panel()->surface().pixels());
              }
            }
          }
        }
      }
    }
  }
}
