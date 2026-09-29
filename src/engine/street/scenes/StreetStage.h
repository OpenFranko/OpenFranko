#ifndef ENGINE_STREET_SCENES_STREETSTAGE_H_
#define ENGINE_STREET_SCENES_STREETSTAGE_H_

#include "../../GameOptions.h"
#include "../../amal/Machine.h"
#include "../../effects/color/AmigaPalette.h"
#include "../core/Bobs.h"
#include "../core/DoubleBuffer.h"
#include "../core/IndexedSurface.h"
#include "../core/LevelScript.h"
#include "../session/GameSession.h"
#include "../ui/LoadingMock.h"
#include "../ui/StageFrame.h"
#include "../ui/StatusPanel.h"
#include "StreetHost.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace scenes {

class StreetStage {
public:
  enum class Outcome { Playing, LevelFinished, GameOver, Quit };

  static constexpr int SCREEN_WIDTH = 320;
  static constexpr int SCREEN_HEIGHT = 222;

  StreetStage(StreetHost &host, session::GameSession &session,
              GameOptions &options);

  void advance(const StreetInput &input);
  void compose(std::vector<uint32_t> &frame) const;
  systems::graphics::Display output() const;

  Outcome outcome() const;
  const core::BobLayer &bobs() const;
  const core::IndexedSurface &screen() const;
  const core::IndexedSurface &display() const;
  const ui::StatusPanel *panel() const;
  amal::Machine &machine();
  int columnsWalked() const;
  int wavesSpawned() const;
  bool isFighting() const;
  bool isScreenShown() const;
  bool isPanelShown() const;

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
  bool bobCol(int number, int first = 0, int last = core::BobLayer::COUNT - 1);
  bool col(int number) const;
  int stage() const;
  ui::StatusPanel::Stats stats() const;
  void stall();
  void autoback(core::DoubleBuffer::Op op);
  bool pasteStalled(int x, int y, int image);
  void putBlock();
  Flow endOfPass() const;
  void playRouted(int request, int voices);

  void newGame();
  void gameInit();
  void openScreens(bool shown);
  void test();
  void hideScreen();
  ui::StageCopper registers() const;
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
  void spawnLoaded();
  void scrollStep();
  void gameOver();
  bool quitsToHighScores() const;
  void closePlayScreen();
  void sys();
  void runBasic(const StreetInput &input);

  StreetHost &m_host;
  session::GameSession &m_session;
  GameOptions &m_options;
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
  core::LevelScript m_script;
  std::vector<core::Picture> m_columns;
  core::Picture m_opening;
  ui::LoadingMock m_loading;
  std::optional<core::ScreenBlock> m_block;

  Step m_step = Step::Start;
  Step m_afterLoading = Step::Finished;
  Outcome m_outcome = Outcome::Playing;
  long m_frame = 0;
  long m_resumeFrame = 0;
  long m_passFrame = 0;
  int m_index = 0;

  int m_energyShown = 0;
  int m_killsShown = 0;
  int m_columnsWalked = 0;
  int m_nextWave = 0;
  int m_wavesSpawned = 0;
  int m_columnInChunk = 0;
  int m_chunk = 0;
  int m_scrollPhase = 0;
  int m_playerX = 0;
  int m_facing = 0;
  int m_screenOffsetX = 0;
  bool m_screenShown = false;
  bool m_panelShown = true;
  bool m_escape = false;
  bool m_mouseButton = false;
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
