#ifndef ENGINE_ENGINESTATEENUM_H_
#define ENGINE_ENGINESTATEENUM_H_

namespace openfranko {
namespace src {
namespace engine {
namespace states {

enum class EngineStateEnum {
  Mirage,
  WorldSoftware,
  KneeAnimation,
  TitleAndStory,
  ProtectionCheck,
  Menu,
  CharacterSelection,
  Level1,
  Level1Boss,
  Level1Car,
  Level2,
  Level2Boss,
  Level2Car,
  StageProtectionCheck,
  Level3,
  GameOver,
  HighScore,
  Continue
};

}
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ENGINESTATEENUM_H_