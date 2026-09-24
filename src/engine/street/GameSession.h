#ifndef ENGINE_STREET_GAMESESSION_H_
#define ENGINE_STREET_GAMESESSION_H_

#include "../amal/Machine.h"
#include "HighScoreTable.h"
#include "IndexedSurface.h"

#include <optional>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

struct StreetExit {
  IndexedSurface screen;
  ScreenBlock block;
  int playerX = 0;
  int energyShown = 0;
  int killsShown = 0;
};

struct GameSession {
  static constexpr int FIRST_EXTRA_LIFE = 35;

  amal::Registers registers{};
  int extraLifeKills = FIRST_EXTRA_LIFE;
  bool brutality = false;
  int stageReached = 0;
  bool fromBonusDrive = false;
  HighScoreTable highScores;
  std::optional<StreetExit> streetExit;
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_GAMESESSION_H_
