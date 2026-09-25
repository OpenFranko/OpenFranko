#ifndef ENGINE_STREET_CARSTAGE_H_
#define ENGINE_STREET_CARSTAGE_H_

#include "../amal/Machine.h"
#include "../effects/AmigaPalette.h"
#include "../effects/GameOptions.h"
#include "Bobs.h"
#include "DoubleBuffer.h"
#include "GameSession.h"
#include "IndexedSurface.h"
#include "LoadingMock.h"
#include "StageFrame.h"
#include "StatusPanel.h"
#include "StreetStage.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

class CarStage {
public:
  enum class Outcome { Playing, DriveFinished, GameOver, Quit };

  static constexpr int SCREEN_WIDTH = 320;
  static constexpr int SCREEN_HEIGHT = 222;
  static constexpr int ROAD_WIDTH = 368;
  static constexpr int CAR = 1;
  static constexpr int FIRST_PEDESTRIAN = 5;
  static constexpr int PEDESTRIANS = 3;
  static constexpr int CAR_CHANNEL = 4;
  static constexpr int PASSWORD_WAIT = 2000;
  static constexpr int DISTANCE = 5000;
  static constexpr int FILES = 3;
  static constexpr int PASSES_PER_SECOND = 15;

  CarStage(StreetHost &host, GameSession &session,
           effects::GameOptions &options);

  void advance(const StreetInput &input);
  void compose(std::vector<uint32_t> &frame) const;

  Outcome outcome() const;
  const BobLayer &bobs() const;
  const IndexedSurface &screen() const;
  const IndexedSurface &display() const;
  const StatusPanel *panel() const;
  bool isScreenShown() const;
  bool isPanelShown() const;
  amal::Machine &machine();
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
    GameOverWait,
    GameOverScreenGone,
    GameOverPanelClose,
    GameOverPanelGone,
    GameOverClosed,
    Finished
  };
  enum class Flow { Continue, Yield };

  int16_t &global(int index);
  int stage() const;
  StatusPanel::Stats stats() const;
  Flow wait(int frames, Step next);
  Flow hold(int frames, Step next);
  Flow autoback(DoubleBuffer::Op op, Step next);
  bool holdsAtStart() const;
  bool holdsAtEnd() const;
  void play(int voices, int sample);
  void loseEnergy(int amount);
  void gainEnergy(int amount);

  void clearScreen();
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
  Flow closePlayScreen();
  void sys();
  void test();
  StageCopper registers() const;
  void runBasic(const StreetInput &input);

  StreetHost &m_host;
  GameSession &m_session;
  effects::GameOptions &m_options;
  amal::Machine m_machine;
  ImageBank m_images;
  BobLayer m_bobs;
  IndexedSurface m_screen;
  DoubleBuffer m_buffer;
  IndexedSurface m_road;
  IndexedSurface m_strip;
  Picture m_backdrop;
  std::unique_ptr<StatusPanel> m_panel;
  amal::Object m_screenDisplay;
  StageDisplay m_copper;
  effects::AmigaPalette m_palette;
  effects::AmigaPalette m_panelPalette;
  LoadingMock m_loading;

  Step m_step = Step::Password;
  Step m_afterLoading = Step::Finished;
  Outcome m_outcome = Outcome::Playing;
  long m_frame = 0;
  long m_resumeFrame = 0;
  long m_holdStart = -1;
  long m_holdUntil = -1;
  SystemKey m_pendingKey = SystemKey::None;
  bool m_escape = false;
  bool m_screenShown = true;
  bool m_panelShown = true;
  int m_screenOffsetX = 0;
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

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CARSTAGE_H_
