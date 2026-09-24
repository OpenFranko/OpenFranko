#ifndef ENGINE_STREET_STREETSTAGE_H_
#define ENGINE_STREET_STREETSTAGE_H_

#include "../amal/Machine.h"
#include "../effects/AmigaPalette.h"
#include "../effects/GameOptions.h"
#include "Bobs.h"
#include "GameSession.h"
#include "IndexedSurface.h"
#include "LevelScript.h"
#include "LoadingMock.h"
#include "StageFrame.h"
#include "StatusPanel.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

class StreetHost {
public:
  virtual ~StreetHost() = default;

  virtual std::vector<Picture> loadSpriteSet(int resource, int sampleBank) = 0;
  virtual Picture loadPicture(int resource) = 0;
  virtual effects::AmigaPalette loadPalette(int resource) = 0;
  virtual std::vector<Picture> loadScenery(int resource) = 0;
  virtual LevelScript loadLevelScript(int resource) = 0;
  virtual Picture loadPanelPicture(int part) = 0;
  virtual void loadMusic(int resource) = 0;
  virtual void playMusic() = 0;
  virtual void stopMusic() = 0;
  virtual void setMusicVolume(int volume) = 0;
  virtual void playSample(int bank, int sample, int voices) = 0;
  virtual void setSampleLoop(bool loop) = 0;
  virtual int random(int limit) = 0;
};

enum class SystemKey { None, MusicOn, MusicOff, Escape };

struct StreetInput {
  int16_t joystick = 0;
  SystemKey key = SystemKey::None;
};

class StreetStage {
public:
  enum class Outcome { Playing, LevelFinished, GameOver, Quit };

  static constexpr int SCREEN_WIDTH = 320;
  static constexpr int SCREEN_HEIGHT = 222;
  static constexpr int LOADING_STRIP = 0;
  static constexpr int PANEL_ARTWORK = 1;

  StreetStage(StreetHost &host, GameSession &session,
              effects::GameOptions &options);

  void advance(const StreetInput &input);
  void compose(std::vector<uint32_t> &frame) const;

  Outcome outcome() const;
  const BobLayer &bobs() const;
  const IndexedSurface &screen() const;
  const IndexedSurface &display() const;
  const StatusPanel *panel() const;
  amal::Machine &machine();
  int columnsWalked() const;
  int wavesSpawned() const;
  bool isFighting() const;
  bool isScreenShown() const;

private:
  enum class Step {
    NewGame,
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
    Finished
  };

  enum class Flow { Continue, Yield };

  int16_t &global(int index);
  int16_t &reg(int channel, int index);
  int xBob(int number) const;
  int yBob(int number) const;
  int iBob(int number) const;
  bool bobCol(int number, int first = 0, int last = BobLayer::COUNT - 1);
  bool col(int number) const;
  int stage() const;
  StatusPanel::Stats stats() const;
  void stall();
  Flow endOfPass() const;
  void playRouted(int request, int voices);

  void newGame();
  void gameInit();
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
  LevelScript m_script;
  std::vector<Picture> m_columns;
  Picture m_opening;
  LoadingMock m_loading;
  std::optional<ScreenBlock> m_block;

  Step m_step = Step::NewGame;
  Step m_afterLoading = Step::Finished;
  Outcome m_outcome = Outcome::Playing;
  long m_frame = 0;
  long m_resumeFrame = 0;
  long m_passFrame = 0;
  int m_index = 0;
  SystemKey m_pendingKey = SystemKey::None;

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
  bool m_escape = false;
  std::array<int, 4> m_energy{};
  std::array<int, 4> m_aggression{};
  std::array<int, 4> m_resident{};
  std::array<int, 4> m_needed{};
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_STREETSTAGE_H_
