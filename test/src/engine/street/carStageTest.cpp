#include "../../../../src/engine/street/CarStage.h"
#include <catch2/catch_all.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::street;

namespace {

constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RN = 13;
constexpr int RO = 14;

constexpr int16_t JOY_UP = 1;
constexpr int16_t JOY_LEFT = 4;
constexpr int16_t JOY_RIGHT = 8;
constexpr int16_t JOY_FIRE = 16;

constexpr int PASSWORD_FRAMES = 6;
constexpr int SKIP_FRAME = PASSWORD_FRAMES + 1;
constexpr int LOADED_FRAME =
    SKIP_FRAME + CarStage::FILES * LoadingMock::FILE_FRAMES;
constexpr int DRIVE_FRAME = LOADED_FRAME + 4;

constexpr uint8_t CAR_INK = 3;
constexpr uint8_t BUSH_INK = 4;
constexpr uint8_t PEDESTRIAN_INK = 6;
constexpr uint8_t MARKER = 5;
constexpr int MARKER_COLUMN = 100;

Picture box(int width, int height, int hotX, int hotY, uint8_t color) {
  return Picture{
      width, height, hotX, hotY,
      std::vector<uint8_t>(static_cast<std::size_t>(width * height), color)};
}

Picture road() {
  Picture picture = box(CarStage::ROAD_WIDTH, CarStage::SCREEN_HEIGHT, 0, 0, 0);
  for (int y = 0; y < CarStage::SCREEN_HEIGHT; ++y) {
    picture.pixels[static_cast<std::size_t>(y * CarStage::ROAD_WIDTH +
                                            MARKER_COLUMN)] = MARKER;
  }
  return picture;
}

class FakeHost : public StreetHost {
public:
  std::vector<std::pair<int, int>> spriteSets;
  std::vector<int> pictures;
  std::vector<std::tuple<int, int, int>> samples;
  std::vector<std::tuple<int, int, int, int>> pitched;
  std::deque<int> rolls;
  int musicStops = 0;
  bool hugePedestrians = false;

  std::vector<Picture> loadSpriteSet(int resource, int sampleBank) override {
    spriteSets.emplace_back(resource, sampleBank);
    std::vector<Picture> frames;
    if (resource == 150) {
      for (int image = 1; image <= 6; ++image) {
        frames.push_back(box(144, 66, 72, 65, CAR_INK));
      }
      return frames;
    }
    frames.push_back(box(96, 50, 48, 49, BUSH_INK));
    frames.push_back(box(96, 61, 48, 60, BUSH_INK));
    for (int image = 9; image <= 28; ++image) {
      frames.push_back(hugePedestrians ? box(400, 100, 400, 50, PEDESTRIAN_INK)
                                       : box(48, 78, 19, 77, PEDESTRIAN_INK));
    }
    return frames;
  }

  Picture loadPicture(int resource) override {
    pictures.push_back(resource);
    return road();
  }

  effects::AmigaPalette loadPalette(int) override { return {}; }

  std::vector<Picture> loadScenery(int) override { return {}; }

  LevelScript loadLevelScript(int) override { return LevelScript{}; }

  Picture loadPanelPicture(int part) override {
    return box(304, part == 0 ? 48 : 40, 0, 0, 1);
  }

  void loadMusic(int) override {}

  void playMusic() override {}

  void stopMusic() override { ++musicStops; }

  void setMusicVolume(int) override {}

  void playSample(int bank, int sample, int voices) override {
    samples.emplace_back(bank, sample, voices);
  }

  void playSampleAt(int bank, int sample, int voices, int frequency) override {
    pitched.emplace_back(bank, sample, voices, frequency);
  }

  void setSampleLoop(bool) override {}

  int random(int) override {
    if (rolls.empty()) {
      return 0;
    }
    const int roll = rolls.front();
    rolls.pop_front();
    return roll;
  }

  bool played(int bank, int sample, int voices) const {
    for (const auto &entry : samples) {
      if (entry == std::make_tuple(bank, sample, voices)) {
        return true;
      }
    }
    return false;
  }
};

struct Drive {
  FakeHost host;
  GameSession session;
  effects::GameOptions options;
  std::unique_ptr<CarStage> stage;

  Drive() {
    session.registers[RO] = 1;
    session.registers[RF] = 40;
    session.registers[RG] = 3;
    session.registers[RN] = 12;
  }

  CarStage &start() {
    stage = std::make_unique<CarStage>(host, session, options);
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

  void toTheWheel() {
    start();
    run(PASSWORD_FRAMES);
    run(1, JOY_FIRE);
    run(DRIVE_FRAME - SKIP_FRAME);
  }

  void ignite() {
    run(1, JOY_FIRE);
    run(29);
  }

  void accelerateTo(int speed) {
    runUntil([&] { return stage->speed() == speed; }, 1000, JOY_RIGHT);
  }

  int markerIn(int row) const {
    for (int x = 0; x < CarStage::SCREEN_WIDTH; ++x) {
      if (stage->screen().pixel(x, row) == MARKER) {
        return x;
      }
    }
    return -1;
  }

  int16_t &global(int index) { return session.registers[index]; }
};

} // namespace

SCENARIO("The next stage's password is printed in the system font on the "
         "cleared screen") {
  GIVEN("The boss of stage 1 just beaten") {
    Drive drive;
    CarStage &stage = drive.start();

    THEN("Cls 0 stalls three frames before Text") {
      drive.run(3);
      REQUIRE(stage.isShowingPassword());
      REQUIRE(stage.screen().pixel(124, 105) == 0);
      drive.run(3);
      REQUIRE(stage.screen().pixel(124, 105) == 9);
    }

    THEN("KOD: CENT sits on baseline 111 from x 124 in ink 9") {
      drive.run(PASSWORD_FRAMES);
      REQUIRE(stage.screen().pixel(124, 105) == 9);
      REQUIRE(stage.screen().pixel(127, 105) == 0);
      REQUIRE(stage.screen().pixel(129, 105) == 9);
      REQUIRE(stage.screen().pixel(151, 106) == 9);
      REQUIRE(stage.screen().pixel(124, 112) == 0);
      REQUIRE(stage.screen().pixel(191, 111) == 9);
      REQUIRE(stage.screen().pixel(190, 111) == 0);
    }

    THEN("The panel still shows the run and nothing has been erased") {
      drive.run(PASSWORD_FRAMES);
      REQUIRE(stage.panel() != nullptr);
      REQUIRE(drive.host.musicStops == 0);
      REQUIRE(drive.host.spriteSets.empty());
    }
  }
}

SCENARIO("KLIKER waits 2000 frames for the joystick, then ERA and the loads") {
  GIVEN("The password on screen") {
    Drive drive;
    CarStage &stage = drive.start();
    drive.run(PASSWORD_FRAMES);

    THEN("Without input it holds for 2000 frames") {
      drive.run(2000);
      REQUIRE(stage.isShowingPassword());
      REQUIRE(drive.host.musicStops == 0);
      drive.run(1);
      REQUIRE_FALSE(stage.isShowingPassword());
      REQUIRE(drive.host.musicStops == 1);
    }

    THEN("Any joystick input ends it at once") {
      drive.run(1, JOY_UP);
      REQUIRE(drive.host.musicStops == 1);
      REQUIRE(drive.host.spriteSets ==
              std::vector<std::pair<int, int>>{{150, 2}});
    }

    THEN("The car, stage 1's walkers and road 907 load in that order") {
      drive.run(1, JOY_FIRE);
      drive.run(LOADED_FRAME - SKIP_FRAME);
      REQUIRE(drive.host.spriteSets ==
              std::vector<std::pair<int, int>>{{150, 2}, {149, 0}});
      REQUIRE(drive.host.pictures == std::vector<int>{907});
      REQUIRE_FALSE(stage.isDriving());
    }
  }
}

SCENARIO("The road opens after the screen juggle and the drive begins") {
  GIVEN("The files loaded") {
    Drive drive;
    CarStage &stage = drive.start();
    drive.run(PASSWORD_FRAMES);
    drive.run(1, JOY_FIRE);
    drive.run(LOADED_FRAME - SKIP_FRAME);

    THEN("Screen Open, Screen Open and Screen Close cost four frames") {
      drive.run(3);
      REQUIRE_FALSE(stage.isDriving());
      drive.run(1);
      REQUIRE(stage.isDriving());
    }

    THEN("The bands start 10, 5, 0 and 0 px along, as the main program left "
         "I, J, L and M") {
      drive.run(4);
      REQUIRE(drive.markerIn(94) == MARKER_COLUMN - 10);
      REQUIRE(drive.markerIn(210) == MARKER_COLUMN - 5);
      REQUIRE(drive.markerIn(50) == MARKER_COLUMN);
      REQUIRE(drive.markerIn(150) == MARKER_COLUMN);
    }

    THEN("The car waits at 96,168, the walkers are parked, the bushes set") {
      drive.run(4);
      const BobLayer &bobs = stage.bobs();
      REQUIRE(bobs.x(CarStage::CAR) == 96);
      REQUIRE(bobs.y(CarStage::CAR) == 168);
      REQUIRE(bobs.image(CarStage::CAR) == 1);
      REQUIRE(bobs.x(5) == -1000);
      REQUIRE(bobs.image(7) == 14);
      REQUIRE(bobs.x(3) == 0);
      REQUIRE(bobs.x(4) == 184);
      REQUIRE(bobs.y(4) == 222);
      REQUIRE(stage.display().pixel(96, 150) == CAR_INK);
      REQUIRE(stage.distance() == CarStage::DISTANCE);
    }
  }
}

SCENARIO("Fire at a standstill starts the engine, then right accelerates") {
  GIVEN("The car at the start line") {
    Drive drive;
    drive.toTheWheel();
    CarStage &stage = *drive.stage;

    THEN("Right does nothing while the engine is off") {
      drive.run(8, JOY_RIGHT);
      REQUIRE(stage.speed() == 0);
      REQUIRE(stage.carX() == 96);
    }

    WHEN("Fire is pressed") {
      drive.run(1, JOY_FIRE);

      THEN("The ignition sample plays and Wait 30 freezes the loop") {
        REQUIRE(drive.host.played(2, 4, 1));
        REQUIRE(stage.isEngineOn());
        drive.run(29, JOY_RIGHT);
        REQUIRE(stage.carX() == 96);
        drive.run(1, JOY_RIGHT);
        REQUIRE(stage.carX() == 97);
      }

      AND_WHEN("Right is held") {
        drive.run(29);
        drive.run(4, JOY_RIGHT);

        THEN("The speed rises one step every fourth pass") {
          REQUIRE(stage.speed() == 1);
          drive.run(44, JOY_RIGHT);
          REQUIRE(stage.speed() == 12);
          drive.run(40, JOY_RIGHT);
          REQUIRE(stage.speed() == 12);
          REQUIRE(stage.carX() == 120);
        }

        THEN("The engine note is replayed every second pass, pitched by "
             "speed") {
          drive.run(48, JOY_RIGHT);
          drive.host.pitched.clear();
          drive.run(4, JOY_RIGHT);
          REQUIRE(drive.host.pitched.size() == 2);
          REQUIRE(drive.host.pitched.front() ==
                  std::make_tuple(2, 5, 8, 5000 + 12 * 200));
        }
      }
    }
  }
}

SCENARIO("The four bands scroll at two, four, three and one times the speed") {
  GIVEN("The car moving at speed 1") {
    Drive drive;
    drive.toTheWheel();
    drive.ignite();
    drive.accelerateTo(1);
    const int track = drive.markerIn(94);
    const int pavement = drive.markerIn(210);
    const int fence = drive.markerIn(50);
    const int road = drive.markerIn(150);

    WHEN("One more pass runs at that speed") {
      drive.run(1, JOY_RIGHT);

      THEN("Each band has moved by its own multiple") {
        REQUIRE(drive.markerIn(94) == track - 2);
        REQUIRE(drive.markerIn(210) == pavement - 4);
        REQUIRE(drive.markerIn(50) == fence - 1);
        REQUIRE(drive.markerIn(150) == road - 3);
      }

      THEN("The road band, copied last, paints over rows 95-114 of the track "
           "band and the fence band over its row 93") {
        REQUIRE(drive.markerIn(95) == drive.markerIn(150));
        REQUIRE(drive.markerIn(114) == drive.markerIn(150));
        REQUIRE(drive.markerIn(93) == drive.markerIn(50));
      }
    }
  }
}

SCENARIO("Steering into the kerb bounces the car back and costs 8 energy") {
  GIVEN("The car at top speed") {
    Drive drive;
    drive.toTheWheel();
    drive.ignite();
    drive.accelerateTo(12);
    CarStage &stage = *drive.stage;

    WHEN("Up is held with right") {
      drive.runUntil([&] { return stage.carY() == 124; }, 20,
                     JOY_UP | JOY_RIGHT);

      THEN("Y stops at 124, the car rolls back and the kerb sample plays") {
        REQUIRE(stage.speed() == -1);
        REQUIRE(drive.host.played(2, 7, 1));
        REQUIRE(drive.global(RF) == 32);
      }
    }
  }
}

SCENARIO("Walkers appear off the right edge with their own AMAL") {
  GIVEN("A spawn rolled for the first channel only") {
    Drive drive;
    drive.toTheWheel();
    CarStage &stage = *drive.stage;
    drive.host.rolls = {4, 2, 50, 10, 0, 0};
    drive.run(1);

    THEN("Bob 5 takes image 19 at Rnd(200)+340, 93+Rnd(20)*4") {
      REQUIRE(stage.bobs().x(5) == 390);
      REQUIRE(stage.bobs().y(5) == 133);
      REQUIRE(stage.bobs().image(5) == 19);
      REQUIRE(stage.machine().isRunning(1));
      REQUIRE_FALSE(stage.machine().exists(2));
    }

    THEN("A running walker is left alone while an idle channel spawns") {
      drive.host.rolls = {4, 4, 0, 0, 0, 0};
      drive.run(1);
      REQUIRE(stage.bobs().x(5) == 390);
      REQUIRE(stage.bobs().image(5) == 20);
      REQUIRE(stage.bobs().x(6) == 340);
      REQUIRE(stage.bobs().y(6) == 93);
      REQUIRE(stage.bobs().image(6) == 9);
      REQUIRE(stage.machine().isRunning(2));
    }
  }
}

SCENARIO("Running a walker down quarters the speed and moves the energy") {
  GIVEN("Walkers big enough to meet the car") {
    Drive drive;
    drive.host.hugePedestrians = true;
    drive.toTheWheel();
    drive.ignite();
    drive.accelerateTo(8);
    CarStage &stage = *drive.stage;

    WHEN("An old man is spawned in the car's lane") {
      drive.host.rolls = {4, 3, 50, 11, 0, 0};
      drive.run(2, JOY_RIGHT);

      THEN("He is hit once: speed 8/4, both samples, R4 set, 14 energy lost") {
        REQUIRE(stage.speed() == 2);
        REQUIRE(drive.host.played(2, 3, 2));
        REQUIRE(drive.host.played(2, 1, 1));
        REQUIRE(stage.machine().channelRegister(1, 4) == 1);
        REQUIRE(drive.global(RF) == 26);
        drive.run(1, JOY_RIGHT);
        REQUIRE(drive.global(RF) == 26);
      }
    }

    WHEN("A punk is spawned in the car's lane") {
      drive.host.rolls = {4, 0, 50, 11, 0, 0};
      drive.run(2, JOY_RIGHT);

      THEN("Running him down gives 14 energy") {
        REQUIRE(drive.global(RF) == 54);
      }
    }
  }
}

SCENARIO("When the distance runs out the car drives off and the stage ends") {
  GIVEN("A drive at top speed") {
    Drive drive;
    drive.toTheWheel();
    drive.ignite();
    CarStage &stage = *drive.stage;
    drive.runUntil([&] { return stage.distance() == 0; }, 1000, JOY_RIGHT);

    THEN("Channel 4 takes the car 800 px right over 400 frames") {
      REQUIRE(stage.machine().isRunning(CarStage::CAR_CHANNEL));
      const int from = stage.bobs().x(CarStage::CAR);
      drive.run(400);
      REQUIRE(stage.bobs().x(CarStage::CAR) == from + 800);
    }

    THEN("Screen Close 5 and Cls 0 follow the move, then the next stage") {
      const int frames = drive.runUntil(
          [&] { return stage.outcome() != CarStage::Outcome::Playing; }, 1000);
      REQUIRE(frames == 406);
      REQUIRE(stage.outcome() == CarStage::Outcome::DriveFinished);
      REQUIRE_FALSE(stage.bobs().isActive(CarStage::CAR));
      REQUIRE(stage.screen().pixel(100, 100) == 0);
      REQUIRE(drive.global(RO) == 1);
    }
  }
}

SCENARIO("Esc and the last life end the drive as state 19 does") {
  GIVEN("A drive under way") {
    Drive drive;
    drive.toTheWheel();
    CarStage &stage = *drive.stage;

    WHEN("Esc is pressed") {
      drive.run(1, 0, SystemKey::Escape);
      drive.run(3);

      THEN("The score is thrown away and the game quits") {
        REQUIRE(stage.outcome() == CarStage::Outcome::Quit);
        REQUIRE(drive.global(RN) == 0);
        REQUIRE(drive.global(RO) == -1);
        REQUIRE(drive.session.stageReached == 1);
      }
    }

    WHEN("The lives run out") {
      drive.global(RG) = -1;
      drive.run(3);

      THEN("Game over follows Wait 200") {
        REQUIRE(stage.outcome() == CarStage::Outcome::Playing);
        drive.run(199);
        REQUIRE(stage.outcome() == CarStage::Outcome::Playing);
        drive.run(1);
        REQUIRE(stage.outcome() == CarStage::Outcome::GameOver);
      }
    }
  }
}
