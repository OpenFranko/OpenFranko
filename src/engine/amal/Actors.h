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

struct DialoguePrograms {
  std::string player;
  std::string boss;
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
PlayerPrograms bossPlayer(int stage);
EnemyPrograms boss(int stage);
std::string spectator(int stage);
DialoguePrograms dialogue(int stage);
std::string walkToBoss();
std::string finishingPose();
std::string finishingBlood();
std::string finishingPoseBack();
std::string walkOff();
std::string bossThrown();
std::string victoryLift();
std::string bossRests();
std::string bubbleUntilFire();
std::string pointingHand();
std::string pedestrian(int image);
std::string carDriveOff();

} // namespace actors
} // namespace amal
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_AMAL_ACTORS_H_
