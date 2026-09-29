#ifndef ENGINE_STREET_SESSION_GAMESESSION_H_
#define ENGINE_STREET_SESSION_GAMESESSION_H_

#include "../../GameVersion.h"
#include "../../InkeyBuffer.h"
#include "../../amal/Machine.h"
#include "../../effects/color/AmigaPalette.h"
#include "../core/DoubleBuffer.h"
#include "../core/HighScoreTable.h"
#include "../core/IndexedSurface.h"
#include "../ui/StageFrame.h"

#include <optional>
#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace session {

inline constexpr int FULL_ENERGY = 64;

enum class SystemKey {
  None,
  Other,
  MusicOn,
  MusicOff,
  Pal,
  Ntsc,
  Lives,
  Escape
};

struct StreetExit {
  core::IndexedSurface screen;
  core::ScreenBlock block;
  int playerX = 0;
  int energyShown = 0;
  int killsShown = 0;
  std::optional<core::DoubleBuffer> buffer;
};

struct BossExit {
  core::DoubleBuffer buffer;
  effects::color::AmigaPalette palette;
  int displayY = 0;
  int offsetX = 0;
  core::IndexedSurface panel;
  int panelY = ui::PANEL_DISPLAY_Y;
  bool laced = false;
};

struct DriveCarryOver {
  int ignition = 0;
  int roadBand = 0;
  int fenceBand = 0;
  int clock = 0;
  int engineBeat = 0;
};

struct GameSession {
  static constexpr int FIRST_EXTRA_LIFE = 35;

  static amal::Registers freshRegisters();

  GameVersion version = GameVersion::V10;
  amal::Registers registers = freshRegisters();
  int extraLifeKills = FIRST_EXTRA_LIFE;
  bool brutality = false;
  bool shortLevels = false;
  std::string textBuffer = core::HighScoreTable::FILE_NAME;
  int stageReached = 0;
  bool fromBonusDrive = false;
  bool nameScreenOpen = false;
  effects::color::AmigaColor border = 0x000;
  SystemKey keyLatch = SystemKey::None;
  DriveCarryOver lastDrive;
  core::HighScoreTable highScores;
  InkeyBuffer keyboard;
  std::optional<StreetExit> streetExit;
  std::optional<BossExit> bossExit;
};

} // namespace session
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_SESSION_GAMESESSION_H_
