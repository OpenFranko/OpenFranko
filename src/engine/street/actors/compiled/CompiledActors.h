#ifndef ENGINE_STREET_ACTORS_COMPILED_COMPILEDACTORS_H_
#define ENGINE_STREET_ACTORS_COMPILED_COMPILEDACTORS_H_

#include "../../../GameVersion.h"
#include "../../../amal/Program.h"

#include <array>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace actors {
namespace compiled {

inline constexpr std::array<GameVersion, 2> VERSIONS = {GameVersion::V10,
                                                        GameVersion::V12};
inline constexpr std::array<int, 3> STAGES = {1, 2, 3};
inline constexpr std::array<int, 4> ENEMY_IMAGE_BASES = {0, 25, 50, 75};
inline constexpr std::array<int, 3> ENEMY_TYPES = {0, 1, 2};
inline constexpr std::array<int, 2> ARROW_FACINGS = {0, 32768};
inline constexpr std::array<int, 4> PEDESTRIAN_IMAGES = {9, 14, 19, 24};
inline constexpr std::array<int, 3> PORTRAITS = {1, 2, 3};

struct PlayerPrograms {
  amal::Program locomotion;
  amal::Program damage;
  amal::Program clamp;
};

struct EnemyPrograms {
  amal::Program walk;
  amal::Program damage;
};

struct DialoguePrograms {
  amal::Program player;
  amal::Program boss;
};

amal::Program playerBlood();
amal::Program enemyBlood(GameVersion version = GameVersion::V10);
amal::Program screenShake(GameVersion version = GameVersion::V10);
PlayerPrograms streetPlayer(int stage, GameVersion version = GameVersion::V10);
EnemyPrograms enemy(int imageBase, int type);
amal::Program idle();
amal::Program indicatorArrow(int facing);
PlayerPrograms bossPlayer(int stage);
EnemyPrograms boss(int stage);
amal::Program spectator(int stage);
DialoguePrograms dialogue(int stage);
amal::Program walkToBoss();
amal::Program finishingPose();
amal::Program finishingBlood();
amal::Program finishingPoseBack();
amal::Program walkOff();
amal::Program bossThrown();
amal::Program victoryLift();
amal::Program bossRests();
amal::Program bubbleUntilFire();
amal::Program walkAway();
amal::Program breakDance();
amal::Program portraitEntrance(int portrait);
amal::Program danceFinale();
amal::Program portraitShuttle(int portrait);
amal::Program pointingHand();
amal::Program pedestrian(int image);
amal::Program carDriveOff();

} // namespace compiled
} // namespace actors
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_ACTORS_COMPILED_COMPILEDACTORS_H_
