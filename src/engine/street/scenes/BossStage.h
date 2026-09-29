#ifndef ENGINE_STREET_SCENES_BOSSSTAGE_H_
#define ENGINE_STREET_SCENES_BOSSSTAGE_H_

#include "../../amal/Machine.h"
#include "../../effects/color/AmigaPalette.h"
#include "../../effects/core/GameOptions.h"
#include "../core/Bobs.h"
#include "../core/DoubleBuffer.h"
#include "../core/IndexedSurface.h"
#include "../ui/GameSession.h"
#include "../ui/LoadingMock.h"
#include "../ui/StageFrame.h"
#include "../ui/StatusPanel.h"
#include "StreetStage.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class BossStage {
public:
  enum class Outcome { Playing, BossDefeated, GameOver, Quit };

  static constexpr int SCREEN_WIDTH = 320;
  static constexpr int SCREEN_HEIGHT = 222;
  static constexpr int APPROACH_COLUMNS = 19;
  static constexpr int BOSS_ENERGY = 80;

  BossStage(StreetHost &host, ui::GameSession &session,
            effects::core::GameOptions &options);

  void advance(const StreetInput &input);
  void compose(std::vector<uint32_t> &frame) const;
  systems::graphics::Display output() const;

  Outcome outcome() const;
  const core::BobLayer &bobs() const;
  const core::IndexedSurface &screen() const;
  const core::IndexedSurface &display() const;
  const ui::StatusPanel *panel() const;
  bool isScreenShown() const;
  bool isPanelShown() const;
  amal::Machine &machine();
  int columnsWalked() const;
  bool isApproaching() const;
  bool isTalking() const;
  bool isFighting() const;
  bool isFinishing() const;
  bool isAtRailing() const;

private:
  enum class Step {
    Init,
    BossMusic,
    BossLoaded,
    Loading,
    Approach,
    ApproachUnpacked,
    ApproachScrolled,
    ChildPasted,
    ChildHit,
    ChildRaised,
    ChildCried,
    Dialogue,
    Fight,
    FightBloodStamped,
    FinishWalkedToBoss,
    FinishPosed,
    FinishStamped,
    FinishPosedBack,
    FinishWalkedOff,
    LiftWalkedToBoss,
    LiftRaised,
    LiftThrown,
    LiftDone,
    RailingSpeech,
    RailingWaitFire,
    RailingReached,
    RailingSat,
    RailingSitting,
    RailingCurse,
    RailingFall,
    RailingFell,
    RailingFallNext,
    RailingQuiet,
    RailingPose,
    RailingGrin,
    RailingGrinned,
    RailingDone,
    Cleared,
    GameOverWait,
    GameOverCleared,
    GameOverScreenGone,
    GameOverPanelClose,
    GameOverPanelGone,
    GameOverClosed,
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
  ui::StatusPanel::Stats stats() const;
  void stall();
  void autoback(core::DoubleBuffer::Op op);
  bool pasteStalled(int x, int y, int image);
  Flow waitFrames(int frames, Step next);
  Flow endOfPass() const;
  void playRequest(int request);

  Flow init();
  Flow bossMusic();
  void bossLoaded();
  Flow load(Step next);
  void setUp();
  Flow approachTop(const StreetInput &input);
  Flow approachScroll();
  Flow approachScrolled();
  Flow approachTail();
  void startDialogue();
  void beginTalk();
  Flow beatChild();
  Flow childRaised();
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
  Flow liftStart();
  Flow liftBoss();
  Flow railingStart();
  Flow railingSpeech();
  Flow railingWaitFire();
  Flow pasteRailing(int image, Step next);
  Flow finishCleanUp();
  void scrollStep();
  void gameOver();
  bool quitsToHighScores() const;
  void closePlayScreen();
  void test();
  ui::StageCopper registers() const;
  void sys();
  void runBasic(const StreetInput &input);

  StreetHost &m_host;
  ui::GameSession &m_session;
  effects::core::GameOptions &m_options;
  amal::Machine m_machine;
  core::ImageBank m_images;
  core::BobLayer m_bobs;
  core::IndexedSurface m_screen;
  core::DoubleBuffer m_buffer;
  std::unique_ptr<ui::StatusPanel> m_panel;
  amal::Object m_screenDisplay;
  ui::StageDisplay m_copper;
  effects::color::AmigaPalette m_palette;
  effects::color::AmigaPalette m_panelPalette;
  std::vector<core::Picture> m_columns;
  ui::LoadingMock m_loading;
  std::optional<core::ScreenBlock> m_block;

  Step m_step = Step::Init;
  Step m_afterLoading = Step::Finished;
  Outcome m_outcome = Outcome::Playing;
  long m_frame = 0;
  long m_resumeFrame = 0;
  long m_passFrame = 0;
  int m_index = 0;
  std::optional<long> m_lastTaunt;

  int m_energyShown = 0;
  int m_killsShown = 0;
  int m_columnsWalked = 0;
  int m_scrollPhase = 0;
  int m_playerX = 0;
  int m_facing = 0;
  int m_screenOffsetX = 0;
  bool m_escape = false;
  bool m_mouseButton = false;
  bool m_screenShown = true;
  bool m_panelShown = true;
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SCENES_BOSSSTAGE_H_
