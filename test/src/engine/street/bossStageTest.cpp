#include "../../../../src/engine/street/BossStage.h"
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
constexpr int RS = 18;
constexpr int RT = 19;
constexpr int RU = 20;
constexpr int RW = 22;
constexpr int RX = 23;

constexpr int16_t JOY_RIGHT = 8;
constexpr int16_t JOY_FIRE = 16;

constexpr int FRANKO_BOSS_SET = 0xFE;
constexpr int FRANKO_EXTRA_PHASES = 0xFD;
constexpr int STAGE_1_BOSS = 0xC8;
constexpr int EXIT_X = 164;
constexpr uint8_t STREET_COLOR = 6;
constexpr uint8_t PLAYER_COLOR = 1;
constexpr uint8_t BOSS_COLOR = 2;

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
    } else if (resource == FRANKO_EXTRA_PHASES) {
      for (int i = 0; i < 2; ++i) {
        frames.push_back(box(64, 80, 16, 77, PLAYER_COLOR));
      }
    } else {
      for (int i = 0; i < 52; ++i) {
        frames.push_back(box(64, 80, 16, 77, BOSS_COLOR));
      }
    }
    return frames;
  }

  Picture loadPicture(int) override { return box(320, 222, 0, 0, 0); }

  std::vector<Picture> loadScenery(int resource) override {
    scenery.push_back(resource);
    std::vector<Picture> columns;
    for (int i = 0; i < 19; ++i) {
      columns.push_back(box(16, 222, 0, 0, columnColor(i)));
    }
    return columns;
  }

  LevelScript loadLevelScript(int) override { return LevelScript{}; }

  Picture loadPanelPicture(int part) override {
    return part == 0 ? box(304, 48, 0, 0, 7) : box(304, 40, 0, 0, 1);
  }

  void playMusic(int resource) override { music.push_back(resource); }

  void setMusicVolume(int volume) override { volumes.push_back(volume); }

  void playSample(int bank, int sample, int voices) override {
    samples.push_back({bank, sample, voices});
  }

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
    session.streetExit.emplace(StreetExit{screen, EXIT_X, 64, 0});
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

  int reachDialogue() {
    return runUntil([this] { return stage->isTalking(); }, 1000, JOY_RIGHT);
  }

  int reachFight() {
    reachDialogue();
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

    THEN("Music 604, the 19 approach columns and the boss sets are loaded") {
      REQUIRE(duel.host.music == std::vector<int>{604});
      REQUIRE(duel.host.volumes.front() == 30);
      REQUIRE(duel.host.scenery == std::vector<int>{320});
      REQUIRE(duel.host.spriteSets ==
              std::vector<std::pair<int, int>>{{0, 0},
                                               {FRANKO_BOSS_SET, 2},
                                               {FRANKO_EXTRA_PHASES, 0},
                                               {STAGE_1_BOSS, 4}});
    }

    THEN("The street's screen comes back with the player where he stopped") {
      REQUIRE(stage.screen().pixel(0, 0) == STREET_COLOR);
      REQUIRE(stage.bobs().x(1) == EXIT_X);
      REQUIRE(stage.bobs().y(1) == 172);
      REQUIRE(stage.bobs().image(1) == 17);
      REQUIRE_FALSE(duel.session.streetExit.has_value());
    }

    WHEN("The two Put Blocks have stalled BASIC for three VBLs each") {
      duel.run(7);

      THEN("Every actor of the duel runs on its channel") {
        for (int channel : {0, 1, 2, 3, 4, 5, 6, 7, 8, 13, 14, 15}) {
          REQUIRE(stage.machine().exists(channel));
        }
        REQUIRE(stage.machine().channelRegister(1, 2) == 164);
        REQUIRE(stage.machine().channelRegister(3, 0) == 272);
        REQUIRE(duel.global(RI) == 1);
      }

      THEN("The boss waits off-screen and the spectator is hidden") {
        REQUIRE(stage.bobs().x(2) == 470);
        REQUIRE(stage.bobs().y(2) == 108);
        REQUIRE(stage.bobs().image(2) == 78);
        REQUIRE(stage.bobs().x(3) == 176);
        REQUIRE(stage.bobs().image(3) == 10);
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

SCENARIO("Walking to the boss scrolls nineteen columns as state 14 does") {
  GIVEN("The approach") {
    Duel duel;
    BossStage &stage = duel.start();
    duel.run(8);

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
    duel.run(8);
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
    duel.run(8);
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

      THEN("Game over follows state 19's Wait 200") {
        REQUIRE(stage.outcome() == BossStage::Outcome::GameOver);
        REQUIRE(ended == 200);
        REQUIRE(duel.global(RO) == -1);
      }
    }
  }
}

SCENARIO("Beating the boss plays KONBOSS and clears the screen") {
  GIVEN("The fight without the brutality cheat") {
    Duel duel;
    BossStage &stage = duel.start();
    duel.run(8);
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
          }
        }
      }
    }
  }

  GIVEN("The fight with the brutality cheat") {
    Duel duel;
    duel.session.brutality = true;
    BossStage &stage = duel.start();
    duel.run(8);
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
