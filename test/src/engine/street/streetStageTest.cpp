#include "../../../../src/engine/street/StreetStage.h"
#include <algorithm>
#include <catch2/catch_all.hpp>
#include <functional>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street;

namespace {

constexpr int RA = 0;
constexpr int RB = 1;
constexpr int RE = 4;
constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RI = 8;
constexpr int RN = 13;
constexpr int RO = 14;
constexpr int RQ = 16;
constexpr int RW = 22;
constexpr int RZ = 25;

constexpr int16_t JOY_RIGHT = 8;
constexpr int16_t JOY_FIRE = 16;

constexpr int FRANKO = 0xFF;
constexpr int ALEX = 0xFA;
constexpr uint8_t OPENING_COLOR = 4;
constexpr uint8_t STRIP_COLOR = 7;
constexpr uint8_t WAIT_WORD_COLOR = 5;
constexpr int OPENING_FILES = 5;
constexpr int OPENING_FRAMES = 1 + OPENING_FILES * LoadingMock::FILE_FRAMES + 3;

Picture box(int width, int height, int hotX, int hotY, uint8_t color) {
  return Picture{
      width, height, hotX, hotY,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height), color)};
}

uint8_t columnColor(int column) { return static_cast<uint8_t>(100 + column); }

EnemySlot enemy(int spriteSet, int x, int y, int energy, int aggression) {
  EnemySlot slot;
  slot.spriteSet = spriteSet;
  slot.type = 0;
  slot.x = x;
  slot.y = y;
  slot.energy = energy;
  slot.aggression = aggression;
  return slot;
}

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

  LevelScript script;
  std::vector<std::pair<int, int>> spriteSets;
  std::vector<int> scenery;
  std::vector<int> music;
  int musicStarts = 0;
  int musicStops = 0;
  std::vector<int> volumes;
  std::vector<Sample> samples;
  int randomCalls = 0;
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
    } else if (resource == FRANKO || resource == ALEX) {
      for (int i = 0; i < 33; ++i) {
        frames.push_back(box(64, 80, 16, 77, 1));
      }
    } else {
      for (int i = 0; i < 24; ++i) {
        frames.push_back(box(32, 77, 16, 75, 2));
      }
      frames.push_back(box(96, 21, 57, 17, 2));
    }
    return frames;
  }

  Picture loadPicture(int) override {
    return box(320, 222, 0, 0, OPENING_COLOR);
  }

  effects::AmigaPalette loadPalette(int) override { return {}; }

  std::vector<Picture> loadScenery(int resource) override {
    scenery.push_back(resource);
    std::vector<Picture> columns;
    for (int i = 0; i < 63; ++i) {
      columns.push_back(box(16, 222, 0, 0, columnColor(i)));
    }
    return columns;
  }

  LevelScript loadLevelScript(int) override { return script; }

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

  void playMusic() override { ++musicStarts; }

  void stopMusic() override { ++musicStops; }

  void setMusicVolume(int volume) override { volumes.push_back(volume); }

  void playSample(int bank, int sample, int voices) override {
    samples.push_back({bank, sample, voices});
  }

  void playSampleAt(int, int, int, int) override {}

  void setSampleLoop(bool) override {}

  int random(int limit) override {
    ++randomCalls;
    return randomValue(limit);
  }

  bool played(int bank, int sample, int voices) const {
    return std::find(samples.begin(), samples.end(),
                     Sample{bank, sample, voices}) != samples.end();
  }
};

struct Street {
  FakeHost host;
  GameSession session;
  effects::GameOptions options;
  std::unique_ptr<StreetStage> stage;

  explicit Street(LevelScript script) { host.script = std::move(script); }

  StreetStage &start() {
    stage = std::make_unique<StreetStage>(host, session, options);
    return *stage;
  }

  void open() { run(OPENING_FRAMES); }

  uint8_t panelPixel(int x, int y) const {
    return stage->panel()->surface().pixel(x, y);
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

  int16_t &global(int index) { return session.registers[index]; }
};

LevelScript emptyStreet(int length) {
  LevelScript script;
  script.length = length;
  return script;
}

LevelScript oneEnemyAt(int trigger, EnemySlot slot, int length = 600) {
  LevelScript script = emptyStreet(length);
  Wave wave;
  wave.trigger = trigger;
  wave.slots[0] = slot;
  script.waves.push_back(wave);
  return script;
}

} // namespace

SCENARIO("A new game opens the street as states 09 and 10 do") {
  GIVEN("Franko chosen with the music on") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.open();

    THEN("The run starts with full energy, three lives and stage 1") {
      REQUIRE(street.global(RF) == 64);
      REQUIRE(street.global(RG) == 3);
      REQUIRE(street.global(RO) == 1);
      REQUIRE(street.global(RN) == 0);
      REQUIRE(street.global(RQ) == 0);
    }

    THEN("Music 601 plays at Mvolume 30, then the shout on all four voices") {
      REQUIRE(street.host.music == std::vector<int>{601});
      REQUIRE(street.host.musicStarts == 1);
      REQUIRE(street.host.volumes.front() == 30);
      REQUIRE(street.host.played(2, 12, 15));
    }

    THEN("Blood loads at image 1 and the player at 11, samples to bank 2") {
      REQUIRE(street.host.spriteSets.size() == 2);
      REQUIRE(street.host.spriteSets[0] == std::make_pair(0, 0));
      REQUIRE(street.host.spriteSets[1] == std::make_pair(FRANKO, 2));
    }

    THEN("The player stands idle at (80, 172) over the opening screen") {
      REQUIRE(stage.bobs().x(1) == 80);
      REQUIRE(stage.bobs().y(1) == 172);
      REQUIRE(stage.bobs().image(1) == 17);
      REQUIRE(stage.screen().pixel(0, 0) == OPENING_COLOR);
    }

    THEN("The double buffer shows the opening at once and the player a VBL "
         "later") {
      REQUIRE(stage.display().pixel(0, 0) == OPENING_COLOR);
      REQUIRE(stage.display().pixel(80, 150) == OPENING_COLOR);
      street.run(1);
      REQUIRE(stage.display().pixel(80, 150) == 1);
    }

    THEN("With no enemies the fight ends at once and the first chunk loads") {
      street.run(1);
      REQUIRE_FALSE(stage.isFighting());
      REQUIRE(street.host.scenery == std::vector<int>{311});
      REQUIRE(street.global(RI) == -1);
    }

    WHEN("The first chunk is loading") {
      street.run(1);

      THEN("Amal Freeze holds the actors while the strip shows") {
        REQUIRE(stage.machine().isFrozen(1));
        REQUIRE(street.panelPixel(101, 10) == STRIP_COLOR);
      }

      AND_WHEN("Its file is in") {
        street.run(LoadingMock::FILE_FRAMES);

        THEN("SCORE redraws the panel and the actors run again") {
          REQUIRE_FALSE(stage.machine().isFrozen(1));
          REQUIRE(street.panelPixel(0, 0) == 1);
        }
      }
    }
  }

  GIVEN("Alex chosen with the music off") {
    Street street(emptyStreet(600));
    street.options.character = effects::Character::Alex;
    street.options.music = false;
    street.start();
    street.open();

    THEN("His street set is loaded and Mvolume is 0") {
      REQUIRE(street.host.spriteSets[1] == std::make_pair(ALEX, 2));
      REQUIRE(street.host.volumes.front() == 0);
      REQUIRE(street.global(RQ) == 1);
    }
  }
}

SCENARIO("Walking right scrolls the street as state 12 does") {
  GIVEN("A street with no waves") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);

    WHEN("Right is held until the scroll has settled") {
      const int reached = street.runUntil(
          [&] { return stage.columnsWalked() == 3; }, 300, JOY_RIGHT);
      std::vector<int> columnFrames;
      int column = stage.columnsWalked();
      for (int frame = 1; frame <= 40; ++frame) {
        street.run(1, JOY_RIGHT);
        if (stage.columnsWalked() != column) {
          column = stage.columnsWalked();
          columnFrames.push_back(frame);
        }
      }

      THEN("Each 16 px column takes 9 frames: 3 for a scroll, 6 with an "
           "unpack") {
        REQUIRE(reached > 0);
        REQUIRE(columnFrames.size() >= 4);
        for (std::size_t i = 1; i < columnFrames.size(); ++i) {
          REQUIRE(columnFrames[i] - columnFrames[i - 1] == 9);
        }
      }

      THEN("The walk bias of 6 holds the player still while the world moves") {
        const int x = stage.bobs().x(1);
        REQUIRE(x > 152);
        REQUIRE(x <= 164);
        street.run(27, JOY_RIGHT);
        REQUIRE(stage.bobs().x(1) == x);
      }

      AND_WHEN("The walk stops") {
        street.run(10);

        THEN("The bias register is cleared again") {
          REQUIRE(stage.machine().channelRegister(1, 1) == 0);
        }
      }
    }

    WHEN("The first column has been scrolled fully into view") {
      street.runUntil([&] { return stage.columnsWalked() == 1; }, 300,
                      JOY_RIGHT);
      street.run(6, JOY_RIGHT);

      THEN("It sits at x 288 to 303, the last visible column") {
        REQUIRE(stage.screen().pixel(288, 100) == columnColor(0));
        REQUIRE(stage.screen().pixel(303, 100) == columnColor(0));
        REQUIRE(stage.screen().pixel(287, 100) == OPENING_COLOR);
      }
    }
  }
}

SCENARIO("A wave spawns at its trigger column") {
  GIVEN("A bald enemy from set 1 due at column 2") {
    Street street(oneEnemyAt(2, enemy(1, 300, 172, 20, 100)));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);
    const int spawned = street.runUntil(
        [&] { return stage.wavesSpawned() == 1; }, 400, JOY_RIGHT);

    THEN("The fight starts with the enemy parked at its spawn point") {
      REQUIRE(spawned > 0);
      REQUIRE(stage.isFighting());
      REQUIRE(stage.bobs().x(2) == 300);
      REQUIRE(stage.bobs().y(2) == 172);
      REQUIRE(street.global(RI) == 1);
      REQUIRE(stage.machine().channelRegister(5, 7) == 20);
    }

    THEN("Its set goes to slot 1, image 44, with its samples in bank 4") {
      REQUIRE(street.host.spriteSets.back() == std::make_pair(1, 4));
    }

    THEN("The player is idle and the empty slots are parked off-screen") {
      REQUIRE(stage.bobs().image(1) == 17);
      REQUIRE(stage.bobs().x(3) == 1000);
      REQUIRE(stage.bobs().image(3) == 10);
      REQUIRE(stage.machine().exists(6));
    }

    THEN("The shout sounds again as the fight begins") {
      REQUIRE(std::count(street.host.samples.begin(), street.host.samples.end(),
                         FakeHost::Sample{2, 12, 15}) == 2);
    }
  }
}

SCENARIO("The referee resolves a punch and a kill") {
  GIVEN("A weak enemy walking in from the right") {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 0, 100)));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);
    street.runUntil([&] { return stage.wavesSpawned() == 1; }, 400, JOY_RIGHT);

    WHEN("The player punches once the enemy is in reach") {
      street.runUntil(
          [&] { return stage.bobs().x(2) - stage.bobs().x(1) <= 36; }, 500);
      const int killed = street.runUntil([&] { return street.global(RN) == 1; },
                                         400, JOY_FIRE);

      THEN("The damage channel kills it, counting the kill and the wave") {
        REQUIRE(killed > 0);
        REQUIRE(street.global(RI) <= 0);
      }

      AND_WHEN("The blood and the shake have settled") {
        const int walking =
            street.runUntil([&] { return stage.machine().exists(13); }, 600);

        THEN("State 12 frees the enemy channels and starts the arrow") {
          REQUIRE(walking > 0);
          REQUIRE_FALSE(stage.isFighting());
          REQUIRE_FALSE(stage.machine().exists(4));
          REQUIRE(street.global(RZ) == 0);
        }
      }
    }
  }
}

SCENARIO("Sound requests are routed to the sprite sets' banks") {
  GIVEN("A fight in progress") {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 50, 100)));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);
    street.runUntil([&] { return stage.wavesSpawned() == 1; }, 400, JOY_RIGHT);

    WHEN("RW and RE are both set") {
      street.global(RW) = 13;
      street.global(RE) = 16;
      street.run(1);

      THEN("RW plays first on voice 1, rebased into bank 4") {
        REQUIRE(street.host.samples.back() == FakeHost::Sample{4, 2, 1});
        REQUIRE(street.global(RE) == 16);
      }

      AND_WHEN("The next pass comes") {
        street.run(1);

        THEN("RE plays on voice 4, rebased into bank 5") {
          REQUIRE(street.host.samples.back() == FakeHost::Sample{5, 2, 8});
          REQUIRE(street.global(RE) == 0);
        }
      }
    }

    WHEN("A request is for the player's own samples") {
      street.global(RW) = 7;
      street.global(RE) = 0;
      street.run(1);

      THEN("It plays from bank 2 unchanged") {
        REQUIRE(street.host.samples.back() == FakeHost::Sample{2, 7, 1});
      }
    }
  }
}

SCENARIO("A Paste Bob stalls the referee for three VBLs") {
  GIVEN("A fight whose enemy dice are rolled each pass") {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 50, 100)));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);
    street.runUntil([&] { return stage.wavesSpawned() == 1; }, 400, JOY_RIGHT);

    WHEN("The player's blood reaches its splat image") {
      street.global(RZ) = 60;
      street.global(RA) = stage.bobs().x(1);
      street.global(RB) = stage.bobs().y(1);
      std::vector<int> rolls;
      for (int frame = 0; frame < 60 && rolls.size() < 5; ++frame) {
        const int before = street.host.randomCalls;
        street.run(1);
        if (!rolls.empty() || stage.bobs().image(10) == 9) {
          rolls.push_back(street.host.randomCalls - before);
        }
      }

      THEN("The splat is stamped, the next two frames run no referee, then "
           "it resumes") {
        REQUIRE(rolls == std::vector<int>{3 + 5, 0, 0, 3, 3});
        REQUIRE(stage.bobs().image(10) == 10);
      }
    }
  }
}

SCENARIO("Stage init counts on from the RO the menu or continue left") {
  GIVEN("RO left at 1 by a continue after dying on stage 2") {
    Street street(emptyStreet(600));
    street.global(RO) = 1;
    street.start();
    street.open();

    THEN("The run opens stage 2 with its music") {
      REQUIRE(street.global(RO) == 2);
      REQUIRE(street.host.music == std::vector<int>{602});
    }
  }
}

SCENARIO("After the bonus drive STAGE INIT carries the run into stage 2") {
  GIVEN("Stage 1 cleared as Alex with 27 kills, 40 energy and 2 lives") {
    Street street(emptyStreet(600));
    street.session.fromBonusDrive = true;
    street.global(RO) = 1;
    street.global(RN) = 27;
    street.global(RF) = 40;
    street.global(RG) = 2;
    street.global(RQ) = 1;
    StreetStage &stage = street.start();
    street.open();

    THEN("State 09 is skipped: nothing is reset and the kills stay") {
      REQUIRE(street.global(RO) == 2);
      REQUIRE(street.global(RN) == 27);
      REQUIRE(street.global(RF) == 40);
      REQUIRE(street.global(RG) == 2);
      REQUIRE(street.global(RQ) == 1);
      REQUIRE_FALSE(street.session.fromBonusDrive);
    }

    THEN("Stage 2's music plays and Alex loads, facing left at X 224") {
      REQUIRE(street.host.music == std::vector<int>{602});
      REQUIRE(street.host.spriteSets[1] == std::make_pair(ALEX, 2));
      REQUIRE(stage.bobs().x(1) == 224);
      REQUIRE((static_cast<uint16_t>(stage.bobs().image(1)) & 0x8000) != 0);
    }
  }
}

SCENARIO("After the second drive STAGE INIT opens stage 3 facing right") {
  GIVEN("Stage 2 cleared and the code card answered") {
    Street street(emptyStreet(600));
    street.session.fromBonusDrive = true;
    street.global(RO) = 2;
    street.global(RN) = 61;
    StreetStage &stage = street.start();
    street.open();

    THEN("Stage 3's music plays, the kills stay and Franko starts at X 80 "
         "facing right") {
      REQUIRE(street.global(RO) == 3);
      REQUIRE(street.global(RN) == 61);
      REQUIRE(street.host.music == std::vector<int>{603});
      REQUIRE(stage.bobs().x(1) == 80);
      REQUIRE((static_cast<uint16_t>(stage.bobs().image(1)) & 0x8000) == 0);
    }
  }
}

SCENARIO("The run ends as state 11 and SYS decide") {
  GIVEN("A fight in progress") {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 50, 100)));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);
    street.runUntil([&] { return stage.wavesSpawned() == 1; }, 400, JOY_RIGHT);

    WHEN("Escape is pressed") {
      street.global(RN) = 5;
      street.run(1, 0, SystemKey::Escape);

      THEN("The score is zeroed and the game quits on the next pass") {
        REQUIRE(street.global(RN) == 0);
        REQUIRE(stage.outcome() == StreetStage::Outcome::Playing);
        street.run(1);
        REQUIRE(stage.outcome() == StreetStage::Outcome::Quit);
        REQUIRE(street.global(RO) == -1);
      }
    }

    WHEN("The last life is lost") {
      street.global(RG) = -2;
      street.run(1);

      THEN("Game over comes after state 19's Wait 200") {
        street.run(199);
        REQUIRE(stage.outcome() == StreetStage::Outcome::Playing);
        street.run(1);
        REQUIRE(stage.outcome() == StreetStage::Outcome::GameOver);
      }

      THEN("ETAP keeps the stage for the continue screen before RO goes") {
        REQUIRE(street.session.stageReached == 1);
        REQUIRE(street.global(RO) == -1);
      }
    }

    WHEN("The kill count reaches KI") {
      street.global(RF) = 20;
      street.global(RN) = 35;
      street.run(1);

      THEN("Energy refills, a life is added and KI moves on by 40") {
        REQUIRE(street.global(RF) == 64);
        REQUIRE(street.global(RG) == 4);
        REQUIRE(street.session.extraLifeKills == 75);
      }
    }
  }

  GIVEN("A street being walked") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1 + LoadingMock::FILE_FRAMES);

    THEN("Escape quits at once") {
      street.run(1, 0, SystemKey::Escape);
      REQUIRE(stage.outcome() == StreetStage::Outcome::Quit);
    }

    THEN("F2 and F1 switch the music off and back on at Mvolume 30") {
      street.run(1, 0, SystemKey::MusicOff);
      REQUIRE(street.host.volumes.back() == 0);
      REQUIRE_FALSE(street.options.music);
      street.run(1, 0, SystemKey::MusicOn);
      REQUIRE(street.host.volumes.back() == 30);
      REQUIRE(street.options.music);
    }

    THEN("RE is played from bank 2 on voice 1 while walking") {
      street.global(RE) = 3;
      street.run(1);
      REQUIRE(street.host.samples.back() == FakeHost::Sample{2, 3, 1});
    }
  }
}

SCENARIO("The level ends one column before its length") {
  GIVEN("A 13 column street") {
    Street street(emptyStreet(13));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);

    WHEN("It is walked to its end") {
      const int ended = street.runUntil(
          [&] {
            return stage.outcome() == StreetStage::Outcome::LevelFinished;
          },
          600, JOY_RIGHT);

      THEN("It stops at column 12, the player stamped where the boss starts") {
        REQUIRE(ended > 0);
        REQUIRE(stage.columnsWalked() == 12);
        REQUIRE_FALSE(stage.bobs().isActive(1));
        REQUIRE_FALSE(stage.machine().exists(1));
      }

      THEN("Nothing moves afterwards") {
        const auto screen = stage.screen().pixels();
        street.run(20, JOY_RIGHT);
        REQUIRE(stage.screen().pixels() == screen);
      }

      THEN("The boss stage gets the stamped screen and the block under it") {
        REQUIRE(street.session.streetExit.has_value());
        const StreetExit &exit = *street.session.streetExit;
        const int x = exit.playerX;
        REQUIRE(x > 152);
        REQUIRE(x <= 164);
        REQUIRE(exit.energyShown == 64);
        REQUIRE(exit.killsShown == 0);
        REQUIRE(exit.screen.pixels() == stage.screen().pixels());
        REQUIRE(exit.screen.pixel(x, 150) == 1);
        IndexedSurface restored = exit.screen;
        exit.block.put(restored);
        REQUIRE(restored.pixel(x, 150) != 1);
        REQUIRE(restored.pixel(x - 17, 150) == exit.screen.pixel(x - 17, 150));
      }
    }
  }
}

SCENARIO("The composed frame shows the play screen over the panel") {
  GIVEN("The street's first playable frame") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.open();
    std::vector<uint32_t> frame;
    stage.compose(frame);

    THEN("It is 304 x 255: 222 play rows, one border row, 32 panel rows") {
      REQUIRE(frame.size() == 304u * 255u);
      REQUIRE(frame[0] == 0xFF008833u);
      REQUIRE(frame[222 * 304] == 0xFF555555u);
      REQUIRE(frame[223 * 304] == 0xFF000000u);
    }
  }

  GIVEN("The first frame of a new game") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.run(1);
    std::vector<uint32_t> frame;
    stage.compose(frame);

    THEN("Screen 0 is still hidden, so only the border and the strip show") {
      REQUIRE(frame[0] == 0xFF555555u);
      REQUIRE(frame[221 * 304 + 303] == 0xFF555555u);
      REQUIRE(frame[(223 + 10) * 304 + 101] == 0xFFDDDDDDu);
    }
  }
}

SCENARIO("A stage's files load one by one as LADUJ and CZEKAJ show them") {
  GIVEN("A new game") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.run(1);

    THEN("ERA has stopped the music and the tune's file is being read") {
      REQUIRE(street.host.musicStops == 1);
      REQUIRE(street.host.music == std::vector<int>{601});
      REQUIRE(street.host.musicStarts == 0);
      REQUIRE(street.host.spriteSets.empty());
      REQUIRE_FALSE(stage.isScreenShown());
      REQUIRE(street.panelPixel(101, 10) == STRIP_COLOR);
    }

    WHEN("The file has been read") {
      street.run(LoadingMock::READ_FRAMES);

      THEN("CZEKAJ puts the wait word over the strip while it unpacks") {
        REQUIRE(street.panelPixel(101, 10) == WAIT_WORD_COLOR);
        REQUIRE(street.panelPixel(100, 10) == STRIP_COLOR);
        REQUIRE(street.host.musicStarts == 0);
      }
    }

    WHEN("The tune has loaded") {
      street.run(LoadingMock::FILE_FRAMES);

      THEN("MUZON starts it and the blood's file is read next") {
        REQUIRE(street.host.musicStarts == 1);
        REQUIRE(street.host.volumes.front() == 30);
        REQUIRE(street.host.spriteSets ==
                std::vector<std::pair<int, int>>{{0, 0}});
        REQUIRE(street.panelPixel(101, 10) == STRIP_COLOR);
      }
    }

    WHEN("All five files are in") {
      street.run(OPENING_FILES * LoadingMock::FILE_FRAMES);

      THEN("The unpack of the opening screen stalls three VBLs before it "
           "shows") {
        REQUIRE_FALSE(stage.isScreenShown());
        street.run(2);
        REQUIRE_FALSE(stage.isScreenShown());
        street.run(1);
        REQUIRE(stage.isScreenShown());
        REQUIRE(stage.isFighting());
      }
    }
  }
}

SCENARIO("A wave's missing sprite set loads with the player stamped down") {
  GIVEN("A bald enemy from set 1 due at column 2") {
    Street street(oneEnemyAt(2, enemy(1, 300, 172, 20, 100)));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);

    WHEN("The wave's column is reached") {
      const int stopped = street.runUntil(
          [&] { return !stage.bobs().isActive(1); }, 400, JOY_RIGHT);
      const int x = stage.bobs().x(1);
      const int y = stage.bobs().y(1);
      street.run(6);

      THEN("Scroll 3 and Paste Bob have run and the set's file is loading") {
        REQUIRE(stopped > 0);
        REQUIRE(stage.screen().pixel(x, y - 2) == 1);
        REQUIRE(street.host.spriteSets.back() == std::make_pair(1, 4));
        REQUIRE(street.panelPixel(101, 10) == STRIP_COLOR);
        REQUIRE(stage.wavesSpawned() == 0);
        REQUIRE_FALSE(stage.machine().exists(1));
      }

      AND_WHEN("The file is in") {
        const int fighting = street.runUntil([&] { return stage.isFighting(); },
                                             LoadingMock::FILE_FRAMES + 1);

        THEN("Put Block takes the stamp back and the fight starts") {
          REQUIRE(fighting == LoadingMock::FILE_FRAMES);
          REQUIRE(stage.screen().pixel(x, y - 2) != 1);
          REQUIRE(stage.bobs().isActive(1));
          REQUIRE(stage.wavesSpawned() == 1);
        }
      }
    }
  }
}
