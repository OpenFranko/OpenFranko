#ifndef ENGINE_STREET_BOSSSTAGE_H_
#define ENGINE_STREET_BOSSSTAGE_H_

#include "../amal/Machine.h"
#include "../effects/AmigaPalette.h"
#include "../effects/GameOptions.h"
#include "Bobs.h"
#include "GameSession.h"
#include "IndexedSurface.h"
#include "StageFrame.h"
#include "StatusPanel.h"
#include "StreetStage.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

class BossStage {
public:
  enum class Outcome { Playing, BossDefeated, GameOver, Quit };

  static constexpr int SCREEN_WIDTH = 320;
  static constexpr int SCREEN_HEIGHT = 222;
  static constexpr int APPROACH_COLUMNS = 19;
  static constexpr int BOSS_ENERGY = 80;

  BossStage(StreetHost &host, GameSession &session,
            effects::GameOptions &options);

  void advance(const StreetInput &input);
  void compose(std::vector<uint32_t> &frame) const;

  Outcome outcome() const;
  const BobLayer &bobs() const;
  const IndexedSurface &screen() const;
  const IndexedSurface &display() const;
  amal::Machine &machine();
  int columnsWalked() const;
  bool isApproaching() const;
  bool isTalking() const;
  bool isFighting() const;
  bool isFinishing() const;

private:
  enum class Step {
    Init,
    InitRestored,
    InitReady,
    Approach,
    ApproachUnpacked,
    ApproachScrolled,
    Dialogue,
    Fight,
    FightBloodStamped,
    FinishWalkedToBoss,
    FinishPosed,
    FinishStamped,
    FinishPosedBack,
    FinishWalkedOff,
    Cleared,
    GameOverWait,
    Finished
  };

  enum class Flow { Continue, Yield };

  int16_t &global(int index);
  int16_t &reg(int channel, int index);
  int xBob(int number) const;
  int yBob(int number) const;
  int iBob(int number) const;
  bool bobCol(int number);
  bool col(int number) const;
  int stage() const;
  StatusPanel::Stats stats() const;
  void stall();
  Flow waitFrames(int frames, Step next);
  Flow endOfPass() const;
  void playRequest(int request);

  void init();
  void restoreBlock();
  void setUp();
  Flow approachTop(const StreetInput &input);
  Flow approachScroll();
  Flow approachScrolled();
  Flow approachTail();
  void startDialogue();
  Flow dialogue();
  Flow fightTop();
  Flow fightBlood();
  Flow fightBloodStamped();
  Flow fightTail();
  Flow finishStart();
  Flow finishPose();
  Flow finishBlood();
  Flow finishStamp();
  Flow finishWalkOff();
  Flow finishCleanUp();
  void scrollStep();
  void gameOver();
  void sys();
  void runBasic(const StreetInput &input);
  void redraw();

  StreetHost &m_host;
  GameSession &m_session;
  effects::GameOptions &m_options;
  amal::Machine m_machine;
  ImageBank m_images;
  BobLayer m_bobs;
  IndexedSurface m_screen;
  IndexedSurface m_display;
  std::unique_ptr<StatusPanel> m_panel;
  amal::Object m_screenDisplay;
  effects::AmigaPalette m_palette;
  effects::AmigaPalette m_panelPalette;
  std::vector<Picture> m_columns;

  Step m_step = Step::Init;
  Outcome m_outcome = Outcome::Playing;
  long m_frame = 0;
  long m_resumeFrame = 0;
  long m_passFrame = 0;
  int m_index = 0;
  SystemKey m_pendingKey = SystemKey::None;

  int m_energyShown = 0;
  int m_killsShown = 0;
  int m_columnsWalked = 0;
  int m_scrollPhase = 0;
  int m_playerX = 0;
  int m_facing = 0;
  int m_screenOffsetX = 0;
  bool m_escape = false;
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_BOSSSTAGE_H_
