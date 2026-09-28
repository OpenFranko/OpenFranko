#ifndef ENGINE_STREET_LEVELSCRIPT_H_
#define ENGINE_STREET_LEVELSCRIPT_H_

#include <array>
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

struct EnemySlot {
  static constexpr int EMPTY = 255;

  int spriteSet = EMPTY;
  int type = 0;
  int x = 0;
  int y = 0;
  int energy = 0;
  int aggression = 0;
};

struct Wave {
  int trigger = 0;
  std::array<EnemySlot, 3> slots;
};

struct LevelScript {
  int length = 0;
  std::vector<Wave> waves;

  static LevelScript fromJson(const std::string &json);
};

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_LEVELSCRIPT_H_
