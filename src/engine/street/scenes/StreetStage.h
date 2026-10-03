#ifndef ENGINE_STREET_SCENES_STREETSTAGE_H_
#define ENGINE_STREET_SCENES_STREETSTAGE_H_

#include "../core/LevelScript.h"
#include "Stage.h"

#include <array>
#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class StreetStage : public Stage {
public:
  StreetStage(StreetHost &host, session::GameSession &session,
              GameOptions &options);

  void handOver() override;
  int columnsWalked() const;
  int wavesSpawned() const;
  bool isFighting() const;

private:
  enum class Step {
    Start,
    GameInitialized,
    StageMusic,
    StageScreen,
    StageShown,
    Loading,
    Referee,
    RefereeCorpseStamped,
    RefereeBloodStamped,
    AdvanceWait,
    Advance,
    AdvanceChunkLoaded,
    AdvanceScroll,
    AdvanceWalked,
    SpawnFlushed,
    SpawnPasted,
    SpawnLoaded,
    AdvanceLeave,
    AdvanceLeaveFlushed,
    AdvanceLeavePasted,
    GameOver,
    Finished
  };

  bool bobCol(int number, int first = 0, int last = core::BobLayer::BOBS - 1);
  void playRouted(int request, int voices);

  void newGame();
  void gameInit();
  void openScreens(bool shown);
  Flow stageInit();
  Flow stageMusic();
  Flow stageScreen();
  void stageShown();
  Flow load(Step next);
  bool grabPlayer();
  Flow stopForLoading(Step next);
  void streetSetup();
  Flow refereeTop();
  Flow refereeEnemies();
  Flow refereeCorpseStamped();
  Flow refereeMoves();
  Flow refereeBlood();
  Flow refereeBloodStamped();
  void hidePastedBlood(int bob);
  Flow refereeTail();
  Flow advanceWait();
  void advanceSetup();
  Flow advanceTop(const StreetInput &input);
  Flow advanceChunkLoaded(const StreetInput &input);
  Flow advanceWalk(const StreetInput &input);
  Flow advanceScroll();
  Flow advanceWalked();
  Flow advanceTail();
  Flow advanceLeaveFlushed();
  Flow advanceLeavePasted();
  Flow spawnFlushed();
  Flow spawnPasted();
  int imageBase(int spriteSet) const;
  void spawnLoaded();
  void gameOver();
  void runBasic(const StreetInput &input) override;

  core::LevelScript m_script;
  std::vector<core::Picture> m_columns;
  core::Picture m_opening;
  std::optional<core::ScreenBlock> m_block;

  Step m_step = Step::Start;
  Step m_afterLoading = Step::Finished;
  int m_index = 0;

  int m_columnsWalked = 0;
  int m_nextWave = 0;
  int m_wavesSpawned = 0;
  int m_columnInChunk = 0;
  int m_chunk = 0;
  int m_scrollPhase = 0;
  int m_playerX = 0;
  int m_facing = 0;
  std::array<int, 4> m_energy{};
  std::array<int, 4> m_aggression{};
  std::array<int, 4> m_resident{};
  std::array<int, 4> m_needed{};
};

} // namespace scenes
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SCENES_STREETSTAGE_H_
