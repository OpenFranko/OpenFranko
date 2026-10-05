#include "../../../../../src/engine/street/scenes/StreetStage.h"

#include "../../../../../src/systems/input/ControllerSystem.h"
#include "../core/box.h"
#include "FakeStreetHost.h"
#include "StageRunner.h"

#include <catch2/catch_all.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street::scenes;
using namespace openfranko::src::engine::street::session;
using namespace openfranko::src::engine::street::ui;
using namespace openfranko::src::engine::street::core;
using namespace openfranko::src::engine::amal;
using namespace openfranko::src::systems::input;
using namespace openfranko::test::src::engine::street::scenes;
using namespace openfranko::test::src::engine::street::core;

namespace {

constexpr int FRANKO = 0xFF;
constexpr int ALEX = 0xFA;
constexpr uint8_t OPENING_COLOR = 4;
constexpr int OPENING_FILES = 5;
constexpr int GAME_INIT_FRAMES = 1 + 3 + 1;
constexpr int STAGE_OPENING_FRAMES =
    1 + OPENING_FILES * LoadingQueue::FILE_FRAMES + 3;
constexpr int OPENING_FRAMES = GAME_INIT_FRAMES + STAGE_OPENING_FRAMES;
constexpr int SCREEN_SHOW_FRAMES = 2;

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

class SteppedLevelScript : public StreetHost::LevelScriptLoad {
public:
  SteppedLevelScript(LevelScript script, int steps)
      : m_script(std::move(script)), m_steps(steps) {}

  bool step(LevelScript &script) override {
    if (--m_steps > 0) {
      return false;
    }
    script = m_script;
    return true;
  }

private:
  LevelScript m_script;
  int m_steps;
};

class FakeHost : public FakeStreetHost {
public:
  LevelScript script;
  int scriptSteps = 1;
  std::vector<int> scenery;
  int randomCalls = 0;
  std::function<int(int)> randomValue = [](int limit) { return limit; };

  std::unique_ptr<LevelScriptLoad> beginLevelScript(int resource) override {
    if (scriptSteps > 1) {
      return std::make_unique<SteppedLevelScript>(script, scriptSteps);
    }
    return FakeStreetHost::beginLevelScript(resource);
  }

  std::vector<Picture> loadSpriteSet(int resource, int sampleBank) override {
    spriteSets.emplace_back(resource, sampleBank);
    std::vector<Picture> frames;
    if (resource == 0) {
      frames = bloodAndIndicator();
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

  std::vector<Picture> loadScenery(int resource) override {
    scenery.push_back(resource);
    std::vector<Picture> columns;
    for (int i = 0; i < 63; ++i) {
      columns.push_back(box(16, 222, 0, 0, columnColor(i)));
    }
    return columns;
  }

  LevelScript loadLevelScript(int) override { return script; }

  int random(int limit) override {
    ++randomCalls;
    return randomValue(limit);
  }
};

struct Street : StageRunner<Street> {
  FakeHost host;
  GameSession session;
  GameOptions options;
  std::unique_ptr<StreetStage> stage;
  bool afterDrive = false;

  explicit Street(LevelScript script) {
    host.script = std::move(script);
    session.registers[RO] = 0;
  }

  StreetStage &start() {
    afterDrive = session.fromBonusDrive;
    stage = std::make_unique<StreetStage>(host, session, options);
    return *stage;
  }

  void open() { run(afterDrive ? STAGE_OPENING_FRAMES : OPENING_FRAMES); }

  uint8_t panelPixel(int x, int y) const {
    return stage->panel()->surface().pixel(x, y);
  }

  int16_t &global(int index) { return session.registers[index]; }

  int16_t &reg(int channel, int index) {
    return stage->machine().channelRegister(channel, index);
  }

  int gap(int bob) const {
    return std::abs(stage->bobs().x(bob) - stage->bobs().x(1));
  }

  void fight() {
    run(OPENING_FRAMES + 1);
    runUntil([this] { return stage->wavesSpawned() == 1; }, 400, JOY_RIGHT);
  }

  int closeIn(int bob) {
    return runUntil([this, bob] { return gap(bob) <= 32; }, 500);
  }

  void throwOverShoulder(int16_t forward) {
    runUntil([this] { return global(RD) == 6; }, 30, JOY_FIRE | forward);
    runUntil([this] { return global(RV) == 5; }, 40, JOY_UP);
  }

  int screenPixels(uint8_t color) const {
    const std::vector<uint8_t> &pixels = stage->screen().pixels();
    return static_cast<int>(std::count(pixels.begin(), pixels.end(), color));
  }
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

LevelScript waveAt(int trigger, EnemySlot first, EnemySlot second,
                   EnemySlot third) {
  LevelScript script = emptyStreet(600);
  Wave wave;
  wave.trigger = trigger;
  wave.slots = {first, second, third};
  script.waves.push_back(wave);
  return script;
}

} // namespace

SCENARIO("A new game opens the street as states 09 and 10 do") {
  GIVEN("The first frames of a new game") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();

    THEN("State 09's Screen Open, Double Buffer and Screen Open hold BASIC "
         "five VBLs, then View shows the panel and STAGE INIT begins") {
      street.run(1);
      REQUIRE_FALSE(stage.isScreenShown());
      REQUIRE_FALSE(stage.isPanelShown());
      street.run(GAME_INIT_FRAMES - 1);
      REQUIRE_FALSE(stage.isPanelShown());
      REQUIRE(street.global(RO) == 0);
      street.run(1);
      REQUIRE(stage.isPanelShown());
      REQUIRE_FALSE(stage.isScreenShown());
      REQUIRE(street.global(RO) == 1);
    }
  }

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
        street.run(LoadingQueue::FILE_FRAMES);

        THEN("SCORE redraws the panel and the actors run again") {
          REQUIRE_FALSE(stage.machine().isFrozen(1));
          REQUIRE(street.panelPixel(0, 0) == 1);
        }
      }
    }
  }

  GIVEN("Alex chosen with the music off") {
    Street street(emptyStreet(600));
    street.options.character = Character::Alex;
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

SCENARIO("The upcoming frame is the one the next update shows") {
  GIVEN("A street being walked through, scrolling as it goes") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);

    THEN("Each upcoming frame matches the frame shown after the next update") {
      int scrolledFrames = 0;
      for (int frame = 0; frame < 200; ++frame) {
        const openfranko::src::systems::graphics::Display upcoming =
            stage.upcomingOutput();
        const int columns = stage.columnsWalked();
        street.run(1, JOY_RIGHT);
        const openfranko::src::systems::graphics::Display shown =
            stage.output();
        scrolledFrames += stage.columnsWalked() != columns ? 1 : 0;
        REQUIRE(upcoming.layers.size() == shown.layers.size());
        for (std::size_t layer = 0; layer < shown.layers.size(); ++layer) {
          REQUIRE(upcoming.layers[layer].pixels == shown.layers[layer].pixels);
          REQUIRE(upcoming.layers[layer].sourceX ==
                  shown.layers[layer].sourceX);
          REQUIRE(upcoming.layers[layer].sourceY ==
                  shown.layers[layer].sourceY);
          REQUIRE(upcoming.layers[layer].top == shown.layers[layer].top);
        }
      }
      REQUIRE(scrolledFrames > 0);
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

SCENARIO("A wave that loads nothing shows its bobs a VBL after CZEKAJ's") {
  GIVEN("A wave of empty slots due at column 2") {
    Street street(oneEnemyAt(2, EnemySlot{}));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);
    street.runUntil([&] { return stage.wavesSpawned() == 1; }, 400, JOY_RIGHT);
    const int x = stage.bobs().x(1);
    const int y = stage.bobs().y(1) - 22;

    THEN("The CZEKAJ call after Paste Bob uses up that VBL's update, so Put "
         "Block's background shows for two frames before the player") {
      REQUIRE(stage.display().pixel(x, y) == OPENING_COLOR);
      street.run(1);
      REQUIRE(stage.display().pixel(x, y) == OPENING_COLOR);
      street.run(1);
      REQUIRE(stage.display().pixel(x, y) == 1);
    }
  }
}

SCENARIO("TRZES moves the screen when the rebuilt copper list goes live") {
  GIVEN("A street running after its first chunk") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.open();
    street.run(SCREEN_SHOW_FRAMES + 80);
    std::vector<uint32_t> frame;
    street.global(RM) = 1;
    street.run(1);
    stage.compose(frame);
    const uint32_t tickTop = frame[0];
    street.run(1);
    stage.compose(frame);

    THEN("The channel moves Screen Display 8 lines down at its VBL, and the "
         "list the next test point builds shows it a VBL later") {
      REQUIRE(tickTop == 0xFF008833u);
      REQUIRE(frame[0] == 0xFF555555u);
      REQUIRE(frame[8 * 304] == 0xFF008833u);
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

    WHEN("A request is for the third slot's samples") {
      street.global(RW) = 19;
      street.global(RE) = 0;
      street.run(1);

      THEN("It plays from bank 6, rebased past the three sets before it") {
        REQUIRE(street.host.samples.back() == FakeHost::Sample{6, 2, 1});
      }
    }
  }
}

SCENARIO("1.2 leaves the blood bob up after its stain is pasted") {
  GIVEN("A 1.2 fight whose player blood is splashing") {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 50, 100)));
    street.session.version = GameVersion::V12;
    StreetStage &stage = street.start();
    street.fight();
    street.global(RZ) = 60;
    street.global(RA) = stage.bobs().x(1);
    street.global(RB) = stage.bobs().y(1);

    WHEN("The splat is reached and pasted") {
      const int splat =
          street.runUntil([&] { return stage.bobs().image(10) == 9; }, 60);
      street.run(1);
      const int stained = street.screenPixels(3);

      THEN("The bob keeps its splat image until its own program hides it") {
        REQUIRE(splat > 0);
        REQUIRE(stained > 0);
        REQUIRE(stage.bobs().image(10) == 9);
        REQUIRE(street.runUntil([&] { return stage.bobs().image(10) == 10; },
                                20) > 0);
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

SCENARIO("A new game keeps the registers state 05 and the menu left") {
  GIVEN("Nine lives from the DOMAN code and 27 kills from the last run") {
    Street street(emptyStreet(600));
    street.global(RG) = 9;
    street.global(RN) = 27;
    street.start();
    street.open();

    THEN("State 09 only clears the kills before stage init counts on") {
      REQUIRE(street.global(RG) == 9);
      REQUIRE(street.global(RF) == 64);
      REQUIRE(street.global(RN) == 0);
      REQUIRE(street.global(RO) == 1);
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

      THEN("The score is zeroed and the next pass's _CLOSE shuts the play "
           "screen, then the panel, four VBLs each, each gone after two") {
        REQUIRE(street.global(RN) == 0);
        REQUIRE(stage.outcome() == StreetStage::Outcome::Playing);
        street.run(2);
        REQUIRE(stage.isScreenShown());
        street.run(1);
        REQUIRE_FALSE(stage.isScreenShown());
        REQUIRE(stage.isPanelShown());
        street.run(3);
        REQUIRE(stage.isPanelShown());
        street.run(1);
        REQUIRE_FALSE(stage.isPanelShown());
        street.run(1);
        REQUIRE(stage.outcome() == StreetStage::Outcome::Playing);
        street.run(1);
        REQUIRE(stage.outcome() == StreetStage::Outcome::Quit);
        REQUIRE(street.global(RO) == -1);
      }
    }

    WHEN("The last life is lost") {
      street.global(RG) = -2;
      street.run(1);

      THEN("After state 19's Wait 200, _CLOSE shuts the play screen and the "
           "panel, two VBLs each, before game over") {
        street.run(200);
        REQUIRE(stage.isScreenShown());
        street.run(2);
        REQUIRE_FALSE(stage.isScreenShown());
        REQUIRE(stage.isPanelShown());
        street.run(4);
        REQUIRE_FALSE(stage.isPanelShown());
        REQUIRE(stage.outcome() == StreetStage::Outcome::Playing);
        street.run(1);
        REQUIRE(stage.outcome() == StreetStage::Outcome::Playing);
        street.run(1);
        REQUIRE(stage.outcome() == StreetStage::Outcome::GameOver);
      }

      THEN("OVER sets RO back to -1") { REQUIRE(street.global(RO) == -1); }
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
    street.run(OPENING_FRAMES + 1 + LoadingQueue::FILE_FRAMES);

    THEN("Escape starts _CLOSE at once and quits once both screens have "
         "closed") {
      street.run(1, 0, SystemKey::Escape);
      REQUIRE(stage.isScreenShown());
      street.run(2);
      REQUIRE_FALSE(stage.isScreenShown());
      street.run(5);
      REQUIRE(stage.outcome() == StreetStage::Outcome::Playing);
      street.run(1);
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

    THEN("F9 gives twelve lives") {
      street.run(1, 0, SystemKey::Lives);
      REQUIRE(street.global(RG) == 12);
      REQUIRE(stage.outcome() == StreetStage::Outcome::Playing);
    }
  }
}

SCENARIO("SYS reads the CIA key register, which keeps the last key event") {
  constexpr int WALKING = OPENING_FRAMES + 1 + LoadingQueue::FILE_FRAMES + 5;

  GIVEN("A new game loading its stage files, when SYS is not called") {
    Street street(emptyStreet(600));
    street.start();
    street.run(GAME_INIT_FRAMES + 10);

    WHEN("F2 is pressed during the loads") {
      street.run(1, 0, SystemKey::MusicOff);
      street.run(WALKING - GAME_INIT_FRAMES - 11);

      THEN("The first SYS after them still switches the music off") {
        REQUIRE_FALSE(street.options.music);
        REQUIRE(street.host.volumes.back() == 0);
      }
    }

    WHEN("Another key is pressed after F2") {
      street.run(1, 0, SystemKey::MusicOff);
      street.run(1, 0, SystemKey::Other);
      street.run(WALKING - GAME_INIT_FRAMES - 12);

      THEN("Its event replaced F2's in the register, so the music stays on") {
        REQUIRE(street.options.music);
        REQUIRE(street.host.volumes.back() == 30);
        REQUIRE(street.session.keyLatch == SystemKey::None);
      }
    }
  }

  GIVEN("A key still in the register when the stage starts") {
    Street street(emptyStreet(600));
    street.session.keyLatch = SystemKey::Ntsc;
    street.start();
    street.run(WALKING);

    THEN("POCZ's Poke $BFEC01,0 has cleared it") {
      REQUIRE_FALSE(street.options.ntsc);
      REQUIRE(street.session.keyLatch == SystemKey::None);
    }
  }
}

SCENARIO("The level ends one column before its length") {
  GIVEN("A 13 column street") {
    Street street(emptyStreet(13));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);

    THEN("Handing it over before its end gives nothing") {
      const auto screen = stage.screen().pixels();
      stage.handOver();
      REQUIRE_FALSE(street.session.streetExit.has_value());
      REQUIRE(stage.screen().pixels() == screen);
    }

    WHEN("It is walked to its end") {
      const int ended = street.runUntil(
          [&] { return stage.outcome() == StreetStage::Outcome::Cleared; }, 600,
          JOY_RIGHT);

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

      THEN("The upcoming frame is the one shown, as no update comes") {
        const uint32_t shown = stage.output().revision;
        REQUIRE(stage.upcomingOutput().revision == shown);
      }

      THEN("Handing it over gives the boss stage the stamped screen and the "
           "block under it") {
        const auto screen = stage.screen().pixels();
        REQUIRE_FALSE(street.session.streetExit.has_value());
        stage.handOver();
        REQUIRE(street.session.streetExit.has_value());
        const StreetExit &exit = *street.session.streetExit;
        REQUIRE(exit.buffer.has_value());
        const int x = exit.playerX;
        REQUIRE(x > 152);
        REQUIRE(x <= 164);
        REQUIRE(exit.energyShown == 64);
        REQUIRE(exit.killsShown == 0);
        REQUIRE(exit.screen.pixels() == screen);
        REQUIRE(exit.screen.pixel(x, 150) == 1);
        IndexedSurface restored = exit.screen;
        exit.block.put(restored);
        REQUIRE(restored.pixel(x, 150) != 1);
        REQUIRE(restored.pixel(x - 17, 150) == exit.screen.pixel(x - 17, 150));
      }
    }
  }
}

SCENARIO("The SKIP code cuts every street to 32 columns as state 10 does") {
  GIVEN("A 600 column street with short levels on") {
    Street street(emptyStreet(600));
    street.session.shortLevels = true;
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);

    WHEN("It is walked to its end") {
      const int ended = street.runUntil(
          [&] { return stage.outcome() == StreetStage::Outcome::Cleared; },
          1000, JOY_RIGHT);

      THEN("It stops at column 31") {
        REQUIRE(ended > 0);
        REQUIRE(stage.columnsWalked() == 31);
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
      REQUIRE(frame[222 * 304] == 0xFF555555u);
      REQUIRE(frame[223 * 304] == 0xFF000000u);
    }

    THEN("The play screen joins them once the list with Screen Show 0 is "
         "live") {
      REQUIRE(frame[0] == 0xFF555555u);
      street.run(SCREEN_SHOW_FRAMES);
      stage.compose(frame);
      REQUIRE(frame[0] == 0xFF008833u);
    }
  }

  GIVEN("The first frame of a new game") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.run(1);
    std::vector<uint32_t> frame;
    stage.compose(frame);

    THEN("Before state 09's View only the border shows") {
      REQUIRE(frame[0] == 0xFF555555u);
      REQUIRE(frame[(223 + 10) * 304 + 101] == 0xFF555555u);
    }

    THEN("Asking again without a change gives the output already built") {
      const uint32_t revision = stage.output().revision;
      street.run(1);
      REQUIRE(stage.output().revision == revision);
      REQUIRE(stage.output().layers.empty());
    }

    WHEN("View has run") {
      street.run(GAME_INIT_FRAMES);
      stage.compose(frame);

      THEN("Screen 0 is still hidden, so only the border and the strip show") {
        REQUIRE(frame[0] == 0xFF555555u);
        REQUIRE(frame[221 * 304 + 303] == 0xFF555555u);
        REQUIRE(frame[(223 + 10) * 304 + 101] == 0xFFDDDDDDu);
      }
    }
  }
}

SCENARIO("F4 and F3 switch the display as SYS does") {
  GIVEN("A PAL street being walked") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1 + LoadingQueue::FILE_FRAMES);
    std::vector<uint32_t> pal;
    stage.compose(pal);

    WHEN("F4 is pressed") {
      street.run(1, 0, SystemKey::Ntsc);
      for (int frame = 0; frame < 20 && !street.options.ntsc; ++frame) {
        street.run(1);
      }
      std::vector<uint32_t> sysFrame;
      stage.compose(sysFrame);
      street.run(1);
      std::vector<uint32_t> beamFrame;
      stage.compose(beamFrame);
      street.run(1);
      std::vector<uint32_t> frame;
      stage.compose(frame);

      THEN("The frame SYS pokes BEAMCON0 in is still PAL") {
        REQUIRE(sysFrame[18 * 304] == pal[18 * 304]);
        REQUIRE(sysFrame[223 * 304] == pal[223 * 304]);
      }

      THEN("The next frame is NTSC, with both screens still on their PAL "
           "lines: the play screen starts on line 47 and the panel is below "
           "the raster") {
        REQUIRE(beamFrame[18 * 304] == 0xFF000000u);
        REQUIRE(beamFrame[19 * 304] == 0xFF555555u);
        REQUIRE(beamFrame[40 * 304] == pal[0]);
        REQUIRE(beamFrame[254 * 304] == pal[214 * 304]);
      }

      THEN("Screen Display's new lines go live a VBL after the next test "
           "point: both screens move up 40 lines, so the rows above line 26 "
           "are lost") {
        REQUIRE(frame[18 * 304] == 0xFF000000u);
        REQUIRE(frame[19 * 304] == pal[19 * 304]);
        REQUIRE(frame[222 * 304] == 0xFF555555u);
        REQUIRE(frame[223 * 304] == pal[223 * 304]);
      }

      AND_WHEN("F4 is pressed again") {
        street.run(1, 0, SystemKey::Ntsc);

        THEN("Nothing changes") { REQUIRE(street.options.ntsc); }
      }

      AND_WHEN("F3 is pressed") {
        street.run(1, 0, SystemKey::Pal);
        for (int frame = 0; frame < 20 && street.options.ntsc; ++frame) {
          street.run(1);
        }
        street.run(1);
        stage.compose(beamFrame);
        street.run(1);
        stage.compose(frame);

        THEN("PAL's beam first shows the screens on their NTSC lines, then "
             "the PAL layout is back") {
          REQUIRE_FALSE(street.options.ntsc);
          REQUIRE(beamFrame[0] == pal[40 * 304]);
          REQUIRE(beamFrame[183 * 304] == pal[223 * 304]);
          REQUIRE(frame[18 * 304] == pal[18 * 304]);
          REQUIRE(frame[222 * 304] == pal[222 * 304]);
          REQUIRE(frame[223 * 304] == pal[223 * 304]);
        }
      }
    }
  }

  GIVEN("A street opened with 320x512 chosen") {
    Street street(emptyStreet(600));
    street.options.tallScreen = true;
    StreetStage &stage = street.start();
    street.open();
    std::vector<uint32_t> frame;
    stage.compose(frame);

    THEN("The laced play screen starts on line 107 and the panel on 219") {
      REQUIRE(frame.size() == 304u * 510u);
      REQUIRE(frame[343 * 304] == 0xFF555555u);
      REQUIRE(frame[344 * 304] == 0xFF000000u);
      street.run(SCREEN_SHOW_FRAMES);
      stage.compose(frame);
      REQUIRE(frame[119 * 304] == 0xFF555555u);
      REQUIRE(frame[120 * 304] == 0xFF008833u);
    }
  }
}

SCENARIO("A stage's files load one by one as LADUJ and CZEKAJ show them") {
  GIVEN("A new game past state 09") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.run(GAME_INIT_FRAMES + 1);

    THEN("ERA has stopped the music and the tune's file is being read") {
      REQUIRE(street.host.musicStops == 1);
      REQUIRE(street.host.music == std::vector<int>{601});
      REQUIRE(street.host.musicStarts == 0);
      REQUIRE(street.host.spriteSets.empty());
      REQUIRE_FALSE(stage.isScreenShown());
      REQUIRE(street.panelPixel(101, 10) == STRIP_COLOR);
    }

    WHEN("The file has been read") {
      street.run(LoadingQueue::READ_FRAMES);

      THEN("CZEKAJ puts the wait word over the strip while it unpacks") {
        REQUIRE(street.panelPixel(101, 10) == WAIT_WORD_COLOR);
        REQUIRE(street.panelPixel(100, 10) == STRIP_COLOR);
        REQUIRE(street.host.musicStarts == 0);
      }
    }

    WHEN("The tune has loaded") {
      street.run(LoadingQueue::FILE_FRAMES);

      THEN("MUZON starts it and the blood's file is read next") {
        REQUIRE(street.host.musicStarts == 1);
        REQUIRE(street.host.volumes.front() == 30);
        REQUIRE(street.host.spriteSets ==
                std::vector<std::pair<int, int>>{{0, 0}});
        REQUIRE(street.panelPixel(101, 10) == STRIP_COLOR);
      }
    }

    WHEN("All five files are in") {
      street.run(OPENING_FILES * LoadingQueue::FILE_FRAMES);

      THEN("The unpack of the opening screen stalls three VBLs. CZEKAJ's "
           "test point then comes before Screen Show 0, so the copper list "
           "that shows the screen is built after the next VBL and goes live "
           "a VBL later, with the player already drawn") {
        REQUIRE_FALSE(stage.isScreenShown());
        street.run(2);
        REQUIRE_FALSE(stage.isScreenShown());
        street.run(1);
        REQUIRE(stage.isFighting());
        REQUIRE_FALSE(stage.isScreenShown());
        street.run(SCREEN_SHOW_FRAMES - 1);
        REQUIRE_FALSE(stage.isScreenShown());
        street.run(1);
        REQUIRE(stage.isScreenShown());
        REQUIRE(stage.display().pixel(stage.bobs().x(1),
                                      stage.bobs().y(1) - 22) == 1);
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
                                             LoadingQueue::FILE_FRAMES + 1);

        THEN("Put Block takes the stamp back and the fight starts") {
          REQUIRE(fighting == LoadingQueue::FILE_FRAMES);
          REQUIRE(stage.screen().pixel(x, y - 2) != 1);
          REQUIRE(stage.bobs().isActive(1));
          REQUIRE(stage.wavesSpawned() == 1);
        }
      }
    }
  }
}

SCENARIO("1.2 clears the play area before its game over closes the screens") {
  const auto framesUntilScreenGone = [](GameVersion version, bool &cleared) {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 50, 100)));
    street.session.version = version;
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);
    street.runUntil([&] { return stage.wavesSpawned() == 1; }, 400, JOY_RIGHT);
    street.global(RG) = -2;
    const int frames =
        street.runUntil([&] { return !stage.isScreenShown(); }, 400);
    const std::vector<uint8_t> &pixels = stage.screen().pixels();
    cleared = std::all_of(pixels.begin(), pixels.end(),
                          [](uint8_t pixel) { return pixel == 0; });
    return frames;
  };

  GIVEN("The last life lost in 1.0 and in 1.2") {
    bool version10Cleared = false;
    bool version12Cleared = false;
    const int version10 =
        framesUntilScreenGone(GameVersion::V10, version10Cleared);
    const int version12 =
        framesUntilScreenGone(GameVersion::V12, version12Cleared);

    THEN("1.2's _OFF and Cls 0 hold the close back by Cls's three VBLs") {
      REQUIRE(version10 > 200);
      REQUIRE(version12 == version10 + 3);
    }

    THEN("Only 1.2 blanks the play area first") {
      REQUIRE_FALSE(version10Cleared);
      REQUIRE(version12Cleared);
    }
  }
}

SCENARIO("A level script read in many steps holds the opening back") {
  GIVEN("Two 13 column streets, one whose script takes 70 steps to read") {
    constexpr int STEPS = 70;
    Street plain(emptyStreet(13));
    Street stepped(emptyStreet(13));
    stepped.host.scriptSteps = STEPS;
    StreetStage &plainStage = plain.start();
    StreetStage &steppedStage = stepped.start();

    WHEN("Both stages open") {
      const int plainOpened =
          plain.runUntil([&] { return plainStage.isFighting(); }, 1000);
      const int steppedOpened =
          stepped.runUntil([&] { return steppedStage.isFighting(); }, 1000);

      THEN("The stepped one opens once its last step is taken, the frames "
           "past the file's own time later") {
        REQUIRE(plainOpened == OPENING_FRAMES);
        REQUIRE(steppedOpened ==
                plainOpened + STEPS - (LoadingQueue::FILE_FRAMES + 1));
      }

      THEN("The script it read is the one played") {
        stepped.runUntil(
            [&] {
              return steppedStage.outcome() == StreetStage::Outcome::Cleared;
            },
            600, JOY_RIGHT);
        REQUIRE(steppedStage.columnsWalked() == 12);
      }
    }
  }
}

SCENARIO("Each of the player's moves lands on an enemy in reach") {
  GIVEN("Franko facing an enemy with 50 energy that has walked up to him") {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 50, 100)));
    StreetStage &stage = street.start();
    street.fight();
    street.closeIn(2);
    const int x = stage.bobs().x(1);
    const auto strike = [&street](int16_t joystick) {
      const int landed =
          street.runUntil([&] { return street.reg(5, 0) != 0; }, 20, joystick);
      street.run(1);
      return landed;
    };

    WHEN("He punches") {
      const int landed = strike(JOY_FIRE);

      THEN("The punch takes 3 energy and the enemy's walk freezes") {
        REQUIRE(landed > 0);
        REQUIRE(street.reg(5, 0) == 1);
        REQUIRE(street.reg(5, 7) == 47);
        REQUIRE(stage.machine().isFrozen(4));
      }
    }

    WHEN("He kicks low with fire and down") {
      strike(JOY_FIRE | JOY_DOWN);

      THEN("The low kick takes 2 energy") {
        REQUIRE(street.reg(5, 0) == 2);
        REQUIRE(street.reg(5, 7) == 48);
      }
    }

    WHEN("He kicks with fire and left, against the way he faces") {
      strike(JOY_FIRE | JOY_LEFT);

      THEN("The enemy staggers without losing energy") {
        REQUIRE(street.reg(5, 0) == 3);
        REQUIRE(street.reg(5, 7) == 50);
        REQUIRE(stage.machine().isFrozen(4));
      }
    }

    WHEN("He jumps at it with fire and up") {
      strike(JOY_FIRE | JOY_UP);

      THEN("The flying kick takes 6 energy and sends it off to the right") {
        REQUIRE(street.reg(5, 0) == 4);
        REQUIRE(street.reg(5, 3) == 16);
        REQUIRE(street.reg(5, 7) == 44);
      }
    }

    WHEN("He jumps up kicking with fire, left and down") {
      strike(JOY_FIRE | JOY_LEFT | JOY_DOWN);

      THEN("The jump kick takes 6 energy as well") {
        REQUIRE(street.reg(5, 0) == 5);
        REQUIRE(street.reg(5, 7) == 44);
      }
    }

    WHEN("He grabs it with fire and right") {
      strike(JOY_FIRE | JOY_RIGHT);

      THEN("It is pulled 40 px in front of him and held, without damage") {
        REQUIRE(street.reg(5, 0) == 6);
        REQUIRE(stage.bobs().x(2) == x + 40);
        REQUIRE(street.reg(2, 5) == 1);
        REQUIRE(street.reg(5, 7) == 50);
      }
    }
  }
}

SCENARIO("A hit enemy walks on once its reaction is over, in any slot") {
  for (std::size_t slot = 0; slot < 3; ++slot) {
    GIVEN("An enemy in wave slot " << slot << " punched once") {
      std::array<EnemySlot, 3> slots;
      slots[slot] = enemy(1, 300, 172, 50, 100);
      Street street(waveAt(1, slots[0], slots[1], slots[2]));
      StreetStage &stage = street.start();
      street.fight();
      const int bob = 2 + static_cast<int>(slot);
      const int walk = 2 * bob;
      street.closeIn(bob);
      street.runUntil([&] { return street.reg(walk + 1, 0) != 0; }, 20,
                      JOY_FIRE);

      THEN("Its walk is frozen during the reaction, then restarted with the "
           "reaction's flag cleared") {
        REQUIRE(stage.machine().isFrozen(walk));
        const int walking = street.runUntil(
            [&] { return !stage.machine().isFrozen(walk); }, 100);
        REQUIRE(walking > 0);
        REQUIRE(street.reg(walk + 1, 2) == 0);
        REQUIRE(street.reg(walk + 1, 7) == 47);
      }
    }
  }
}

SCENARIO("Enemies spawned on one spot step out of each other's way") {
  GIVEN("Three enemies spawned on top of each other") {
    const EnemySlot slot = enemy(1, 300, 172, 50, 100);
    Street street(waveAt(1, slot, slot, slot));
    StreetStage &stage = street.start();
    street.fight();
    street.run(2);

    THEN("The first two are told to step aside and the third walks on") {
      REQUIRE(street.reg(4, 3) == 1);
      REQUIRE(street.reg(6, 3) == 1);
      REQUIRE(street.reg(8, 3) == 0);
      street.run(20);
      REQUIRE(stage.bobs().y(2) > 172);
      REQUIRE(stage.bobs().y(3) > 172);
      REQUIRE(stage.bobs().y(4) == 172);
      REQUIRE(stage.bobs().x(4) < 300);
    }
  }
}

SCENARIO("A downed enemy is picked up and kneed until it dies") {
  GIVEN("An enemy floored by a jump kick, Franko just past it facing right") {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 50, 100)));
    StreetStage &stage = street.start();
    street.fight();
    street.run(36, JOY_LEFT);
    street.run(4, JOY_RIGHT);
    street.closeIn(2);
    street.runUntil([&] { return street.global(RD) == 5; }, 20,
                    JOY_FIRE | JOY_LEFT | JOY_DOWN);
    const int floored =
        street.runUntil([&] { return street.reg(5, 4) == 1; }, 200);
    const int past = street.runUntil(
        [&] {
          const int over = stage.bobs().x(1) - stage.bobs().x(2);
          return over > 0 && over < 25;
        },
        200, JOY_RIGHT);

    WHEN("Fire and down are pressed") {
      const int held = street.runUntil([&] { return street.reg(5, 4) == 2; },
                                       10, JOY_FIRE | JOY_DOWN);

      THEN("He holds it 24 px behind him and 4 px up, both of them frozen") {
        REQUIRE(floored > 0);
        REQUIRE(past > 0);
        REQUIRE(held > 0);
        REQUIRE(stage.bobs().x(2) == stage.bobs().x(1) - 24);
        REQUIRE(stage.bobs().y(2) == stage.bobs().y(1) - 4);
        REQUIRE(stage.machine().isFrozen(1));
        REQUIRE(stage.machine().isFrozen(4));
        REQUIRE(street.global(RD) == 2);
      }

      AND_WHEN("The knees have played out") {
        const int freed =
            street.runUntil([&] { return !stage.machine().isFrozen(1); }, 200);
        const int killed =
            street.runUntil([&] { return street.global(RN) == 1; }, 100);

        THEN("He walks again and the enemy dies") {
          REQUIRE(freed > 0);
          REQUIRE(killed > 0);
          REQUIRE(street.global(RI) <= 0);
        }
      }
    }
  }
}

SCENARIO("An enemy that strikes knocks the player back") {
  GIVEN("Dice that make every enemy in reach attack") {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 50, 100)));
    street.host.randomValue = [](int) { return 0; };
    StreetStage &stage = street.start();
    street.fight();

    WHEN("The enemy's first blow lands") {
      int before = 0;
      const int hit = street.runUntil(
          [&] {
            if (street.reg(2, 1) == 1) {
              return true;
            }
            before = stage.bobs().x(2);
            return false;
          },
          300);
      const int x = stage.bobs().x(2);
      street.run(8);

      THEN("The player is frozen, the enemy is set 8 px back and the player "
           "loses 1 energy") {
        REQUIRE(hit > 0);
        REQUIRE(x == before + 8);
        REQUIRE(stage.machine().isFrozen(1));
        REQUIRE(street.global(RD) == 9);
        REQUIRE(street.global(RF) == 63);
      }

      AND_WHEN("The knock-back has played out") {
        const int freed =
            street.runUntil([&] { return !stage.machine().isFrozen(1); }, 100);

        THEN("The player can move again") {
          REQUIRE(freed > 0);
          REQUIRE(street.global(RD) == 0);
          REQUIRE(street.reg(2, 1) == 0);
          REQUIRE(street.reg(2, 4) == 0);
        }
      }
    }

    WHEN("It lands on the last point of energy") {
      street.global(RF) = 1;
      const int lost =
          street.runUntil([&] { return street.global(RG) == 2; }, 300);

      THEN("A life goes and the bar on the panel is full again") {
        REQUIRE(lost > 0);
        REQUIRE(street.global(RF) == 64);
        REQUIRE(street.panelPixel(112, 13) != street.panelPixel(175, 13));
        REQUIRE(street.panelPixel(174, 13) == street.panelPixel(112, 13));
      }
    }
  }
}

SCENARIO("An enemy thrown over the shoulder floors the one it lands on") {
  GIVEN("Franko grabbing the enemy in front of him while another comes up "
        "behind in slot 2") {
    Street street(waveAt(1, enemy(1, 300, 172, 50, 100),
                         enemy(1, -100, 172, 50, 100), EnemySlot{}));
    StreetStage &stage = street.start();
    street.fight();
    street.closeIn(2);
    street.throwOverShoulder(JOY_RIGHT);

    WHEN("The thrown enemy comes down") {
      const int floored =
          street.runUntil([&] { return street.reg(7, 0) == 4; }, 120);
      const int thrownFlag = street.reg(5, 3);
      street.run(1);

      THEN("The one behind is kicked over for 6 energy, and the throw is "
           "spent") {
        REQUIRE(floored > 0);
        REQUIRE(thrownFlag == 0);
        REQUIRE(street.reg(7, 7) == 44);
        REQUIRE(stage.machine().isFrozen(6));
      }
    }
  }

  GIVEN("The same throw while the enemy behind is in slot 3") {
    Street street(waveAt(1, enemy(1, 300, 172, 50, 100),
                         enemy(1, 600, 172, 50, 100),
                         enemy(1, -120, 172, 50, 100)));
    street.start();
    street.fight();
    street.closeIn(2);
    street.throwOverShoulder(JOY_RIGHT);

    THEN("The slot 3 enemy is kicked over") {
      REQUIRE(street.runUntil([&] { return street.reg(9, 0) == 4; }, 120) > 0);
      street.run(1);
      REQUIRE(street.reg(9, 7) == 44);
      REQUIRE(street.reg(5, 3) == 0);
    }
  }

  GIVEN("Franko facing left, throwing the slot 2 enemy onto slot 1's") {
    Street street(waveAt(1, enemy(1, 450, 172, 50, 100),
                         enemy(1, -40, 172, 50, 100), EnemySlot{}));
    StreetStage &stage = street.start();
    street.fight();
    street.run(4, JOY_LEFT);
    street.closeIn(3);
    street.throwOverShoulder(JOY_LEFT);

    THEN("The thrown enemy flies right and floors the slot 1 enemy") {
      REQUIRE(street.global(RC) != 0);
      REQUIRE(stage.bobs().x(3) < stage.bobs().x(1));
      REQUIRE(street.runUntil([&] { return street.reg(5, 0) == 4; }, 120) > 0);
      REQUIRE(street.reg(7, 3) == 0);
      street.run(1);
      REQUIRE(street.reg(5, 7) == 44);
    }
  }
}

SCENARIO("A corpse is stamped only where it falls on the screen") {
  GIVEN("An enemy with no energy left") {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 0, 100)));
    StreetStage &stage = street.start();
    street.fight();

    WHEN("It is punched dead in the street") {
      street.closeIn(2);
      street.runUntil([&] { return street.global(RN) == 1; }, 300, JOY_FIRE);
      street.run(5);

      THEN("Its 96 x 21 corpse is pasted into the screen") {
        REQUIRE(street.screenPixels(2) == 96 * 21);
        REQUIRE(stage.bobs().image(2) == 10);
      }
    }

    WHEN("It is kicked dead past the right edge") {
      street.run(50, JOY_RIGHT);
      street.closeIn(2);
      street.runUntil([&] { return street.global(RN) == 1; }, 300,
                      JOY_FIRE | JOY_UP);
      street.run(5);

      THEN("Nothing is pasted and the bob is hidden") {
        REQUIRE(street.global(RN) == 1);
        REQUIRE(street.screenPixels(2) == 0);
        REQUIRE(stage.bobs().image(2) == 10);
      }
    }
  }
}

SCENARIO("Blood that lands off the screen leaves no stain") {
  GIVEN("Franko at the left edge facing left, an enemy come in from beyond "
        "it") {
    Street street(oneEnemyAt(1, enemy(1, -104, 172, 50, 100)));
    StreetStage &stage = street.start();
    street.fight();
    street.run(80, JOY_LEFT);
    street.closeIn(2);

    WHEN("He punches it") {
      street.runUntil([&] { return street.reg(5, 0) != 0; }, 20, JOY_FIRE);
      const int splashed = street.runUntil(
          [&] {
            return stage.bobs().x(11) < 0 && stage.bobs().y(11) == 172 &&
                   stage.bobs().image(11) == 10;
          },
          60);

      THEN("The blood flies off the left edge and is hidden unstamped") {
        REQUIRE(stage.bobs().x(1) == 32);
        REQUIRE(stage.bobs().x(2) < 16);
        REQUIRE(splashed > 0);
        REQUIRE(street.screenPixels(3) == 0);
      }
    }
  }
}

SCENARIO("Stage 2 is walked to the left") {
  GIVEN("Franko on stage 2, a weak enemy due at column 3") {
    Street street(oneEnemyAt(3, enemy(1, -40, 172, 0, 100)));
    street.global(RO) = 1;
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1);

    WHEN("Left is held until the first column has scrolled in") {
      const int walked = street.runUntil(
          [&] { return stage.columnsWalked() == 1; }, 300, JOY_LEFT);
      street.run(6, JOY_LEFT);

      THEN("He stops at x 158 and the street comes in at the left edge") {
        REQUIRE(walked > 0);
        REQUIRE(stage.bobs().x(1) == 158);
        REQUIRE(stage.screen().pixel(0, 100) == columnColor(0));
        REQUIRE(stage.screen().pixel(40, 100) == OPENING_COLOR);
      }
    }

    WHEN("The enemy is met and punched dead") {
      street.runUntil([&] { return stage.wavesSpawned() == 1; }, 600, JOY_LEFT);
      street.closeIn(2);
      street.runUntil([&] { return street.global(RN) == 1; }, 300, JOY_FIRE);
      street.run(10);

      THEN("Its whole corpse is pasted into the screen") {
        REQUIRE(street.screenPixels(2) == 96 * 21);
      }
    }
  }
}

SCENARIO("The mouse button counts the rest of the wave as killed") {
  GIVEN("A fight against one enemy") {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 50, 100)));
    StreetStage &stage = street.start();
    street.fight();

    WHEN("The mouse button is pressed") {
      for (int frame = 0; frame < 10 && street.global(RN) == 0; ++frame) {
        stage.advance({0, SystemKey::None, true});
      }

      THEN("SYS adds the enemies left to the kills and the walk resumes") {
        REQUIRE(street.global(RN) == 1);
        REQUIRE(street.global(RI) <= 0);
        REQUIRE(street.runUntil([&] { return !stage.isFighting(); }, 100) > 0);
      }
    }
  }

  GIVEN("A street being walked, no wave left") {
    Street street(emptyStreet(600));
    StreetStage &stage = street.start();
    street.run(OPENING_FRAMES + 1 + LoadingQueue::FILE_FRAMES);

    WHEN("The mouse button is pressed") {
      for (int frame = 0; frame < 10; ++frame) {
        stage.advance({0, SystemKey::None, true});
      }

      THEN("Nothing is counted") {
        REQUIRE(street.global(RN) == 0);
        REQUIRE(street.global(RI) == -1);
      }
    }
  }
}

SCENARIO("Bobs shown as sprites look the same as bobs drawn on the street") {
  GIVEN("Two streets with a wave, one on a monitor that shows sprites") {
    Street drawn(oneEnemyAt(2, enemy(1, 300, 172, 0, 100)));
    Street sprited(oneEnemyAt(2, enemy(1, 300, 172, 0, 100)));
    drawn.start();
    sprited.start().showSprites(true);

    WHEN("Both walk into the wave and fight it, sprites switched off for "
         "the last stretch") {
      constexpr int FRAMES = 640;
      constexpr int WALK = OPENING_FRAMES + 200;
      constexpr int DRAWN_FROM = 480;
      int spriteFrames = 0;
      int matched = 0;
      std::vector<uint32_t> drawnFrame;
      std::vector<uint32_t> spritedFrame;
      for (int frame = 0; frame < FRAMES; ++frame) {
        if (frame == DRAWN_FROM) {
          sprited.stage->showSprites(false);
        }
        const int16_t joystick = frame < WALK ? JOY_RIGHT : JOY_FIRE;
        drawn.run(1, joystick);
        sprited.run(1, joystick);
        if (frame % 8 != 0) {
          continue;
        }
        drawn.stage->compose(drawnFrame);
        sprited.stage->compose(spritedFrame);
        matched += spritedFrame == drawnFrame ? 1 : 0;
        const openfranko::src::systems::graphics::Display &shown =
            sprited.stage->output();
        if (!shown.layers.empty() && !shown.layers.front().sprites.empty()) {
          ++spriteFrames;
        }
      }

      THEN("Every frame matched, the bobs riding on the play screen as "
           "sprites until they were switched off") {
        REQUIRE(matched == FRAMES / 8);
        REQUIRE(spriteFrames > 0);
        REQUIRE(sprited.stage->output().layers.front().sprites.empty());
        REQUIRE(drawn.global(RN) == 1);
        REQUIRE(sprited.global(RN) == 1);
      }
    }
  }
}

SCENARIO("Later waves keep the sprite sets already loaded") {
  GIVEN("Three waves: set 1, then sets 1 and 7, then sets 8, 9 and 7") {
    LevelScript script = emptyStreet(600);
    Wave first;
    first.trigger = 1;
    first.slots[0] = enemy(1, 300, 172, 50, 100);
    Wave second;
    second.trigger = 3;
    second.slots[0] = enemy(1, 300, 172, 50, 100);
    second.slots[1] = enemy(7, 330, 172, 50, 100);
    Wave third;
    third.trigger = 5;
    third.slots = {enemy(8, 300, 172, 50, 100), enemy(9, 330, 172, 50, 100),
                   enemy(7, 360, 172, 50, 100)};
    script.waves = {first, second, third};
    Street street(script);
    StreetStage &stage = street.start();
    street.fight();
    const auto skipWave = [&] {
      for (int frame = 0; frame < 20 && street.global(RI) > 0; ++frame) {
        stage.advance({0, SystemKey::None, true});
      }
    };
    const auto reachWave = [&](int wave) {
      return street.runUntil([&] { return stage.wavesSpawned() == wave; }, 1000,
                             JOY_RIGHT);
    };
    const std::size_t opening = street.host.spriteSets.size();

    WHEN("The second wave comes") {
      skipWave();
      const int reached = reachWave(2);

      THEN("Only set 7 is read, into the second slot's bank 5") {
        REQUIRE(reached > 0);
        REQUIRE(street.host.spriteSets.size() == opening + 1);
        REQUIRE(street.host.spriteSets.back() == std::make_pair(7, 5));
      }

      AND_WHEN("The third wave comes") {
        skipWave();
        reachWave(3);

        THEN("Set 8 takes the first slot, set 9 the third, set 7 stays") {
          REQUIRE(street.host.spriteSets.size() == opening + 3);
          REQUIRE(street.host.spriteSets[opening + 1] == std::make_pair(8, 4));
          REQUIRE(street.host.spriteSets[opening + 2] == std::make_pair(9, 6));
        }
      }
    }
  }
}

SCENARIO("In 1.2 Esc ends the run as a game over") {
  GIVEN("A 1.2 fight in progress") {
    Street street(oneEnemyAt(1, enemy(1, 300, 172, 50, 100)));
    street.session.version = GameVersion::V12;
    StreetStage &stage = street.start();
    street.fight();
    street.global(RN) = 5;

    WHEN("Esc is pressed") {
      street.run(1, 0, SystemKey::Escape);
      const int ended = street.runUntil(
          [&] { return stage.outcome() != StreetStage::Outcome::Playing; },
          400);

      THEN("The score is zeroed, but state 19's Wait 200 and the game over "
           "follow instead of the quit") {
        REQUIRE(ended > 200);
        REQUIRE(stage.outcome() == StreetStage::Outcome::GameOver);
        REQUIRE(street.global(RN) == 0);
        REQUIRE(street.global(RO) == -1);
      }
    }
  }
}
