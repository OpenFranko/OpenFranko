#ifndef ENGINE_STREET_GAMESESSION_H_
#define ENGINE_STREET_GAMESESSION_H_

#include "../amal/Machine.h"

namespace openfranko {
namespace src {
namespace engine {
namespace street {

struct GameSession {
  static constexpr int FIRST_EXTRA_LIFE = 35;

  amal::Registers registers{};
  int extraLifeKills = FIRST_EXTRA_LIFE;
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_GAMESESSION_H_
