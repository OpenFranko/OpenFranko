#ifndef ENGINE_STREET_SCENES_BOSSSTAGE_H_
#define ENGINE_STREET_SCENES_BOSSSTAGE_H_

#include "Stage.h"

#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class BossStage : public Stage {
public:
  static constexpr int APPROACH_COLUMNS = 19;
  static constexpr int BOSS_ENERGY = 80;

  BossStage(StreetHost &host, session::GameSession &session,
            GameOptions &options);

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
    GameOver,
    Finished
  };

  bool bobCol(int number);
  Flow waitFrames(int frames, Step next);
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
  void gameOver();
  void runBasic(const StreetInput &input) override;

  std::vector<core::Picture> m_columns;
  std::optional<core::ScreenBlock> m_block;

  Step m_step = Step::Init;
  Step m_afterLoading = Step::Finished;
  int m_index = 0;
  std::optional<long> m_lastTaunt;

  int m_columnsWalked = 0;
  int m_scrollPhase = 0;
  int m_playerX = 0;
  int m_facing = 0;
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SCENES_BOSSSTAGE_H_
