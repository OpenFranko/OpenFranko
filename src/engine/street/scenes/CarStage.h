#ifndef ENGINE_STREET_SCENES_CARSTAGE_H_
#define ENGINE_STREET_SCENES_CARSTAGE_H_

#include "Stage.h"

#include <array>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class CarStage : public Stage {
public:
  static constexpr int ROAD_WIDTH = 368;
  static constexpr int CAR = 1;
  static constexpr int FIRST_PEDESTRIAN = 5;
  static constexpr int PEDESTRIANS = 3;
  static constexpr int CAR_CHANNEL = 4;
  static constexpr int PASSWORD_WAIT = 2000;
  static constexpr int DISTANCE = 5000;
  static constexpr int FILES = 3;
  static constexpr int PASSES_PER_SECOND = 15;

  CarStage(StreetHost &host, session::GameSession &session,
           GameOptions &options);

  bool isShowingPassword() const;
  bool isDriving() const;
  int passes() const;
  int distance() const;
  int speed() const;
  int carX() const;
  int carY() const;
  bool isEngineOn() const;

private:
  enum class Step {
    Password,
    PasswordText,
    Kliker,
    HideForLoading,
    Era,
    Loading,
    Loaded,
    RoadOpened,
    StripOpened,
    RoadClosed,
    DriveTop,
    DriveIgnited,
    DriveScenery,
    DriveBottom,
    StripClosed,
    Cleared,
    GameOver,
    Finished
  };

  Flow wait(int frames, Step next);
  Flow hold(int frames, Step next);
  Flow autoback(core::DoubleBuffer::Op op, Step next);
  void play(int voices, int sample);
  void loseEnergy(int amount);
  void gainEnergy(int amount);

  void clearScreen();
  void hideForLoading();
  void password();
  void era();
  void openRoad();
  void openStrip();
  void startDrive();
  Flow driveTop(const StreetInput &input);
  Flow driveInput(const StreetInput &input);
  int nextPassFrames();
  void hitKerb(int kerb);
  void spawnPedestrians();
  Flow driveScenery();
  Flow driveBottom();
  void runOver();
  Flow leave();
  void gameOver();
  void runBasic(const StreetInput &input) override;

  core::IndexedSurface m_road;
  core::IndexedSurface m_strip;
  core::Picture m_backdrop;

  Step m_step = Step::Password;
  Step m_afterLoading = Step::Finished;
  int m_waited = 0;

  int m_x = 0;
  int m_y = 0;
  int m_distance = 0;
  int m_trail = 0;
  int m_topSpeed = 0;
  int m_pull = 0;
  int m_speed = 0;
  int m_ignition = 0;
  int m_horn = 0;
  int m_accel = 0;
  int m_steer = 0;
  int m_bush1 = 0;
  int m_bush2 = 0;
  int m_fenceBand = 0;
  int m_trackBand = 0;
  int m_roadBand = 0;
  int m_pavementBand = 0;
  int m_clock = 0;
  int m_engineBeat = 0;
  int m_passes = 0;
  int m_passTime = 0;
  std::array<bool, PEDESTRIANS + 1> m_hit{};
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SCENES_CARSTAGE_H_
