#ifndef ENGINE_AMAL_ACTORS_H_
#define ENGINE_AMAL_ACTORS_H_

#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace amal {
namespace actors {

struct PlayerPrograms {
  std::string locomotion;
  std::string damage;
  std::string clamp;
};

struct EnemyPrograms {
  std::string walk;
  std::string damage;
};

int amosBool(bool condition);
std::string hex(int value);

std::string playerBlood();
std::string enemyBlood();
std::string screenShake();
PlayerPrograms streetPlayer(int stage);
EnemyPrograms enemy(int imageBase, int type);
std::string idle();
std::string indicatorArrow(int facing);

} // namespace actors
} // namespace amal
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_AMAL_ACTORS_H_
