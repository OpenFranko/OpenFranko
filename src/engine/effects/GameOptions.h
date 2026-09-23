#ifndef ENGINE_EFFECTS_GAMEOPTIONS_H_
#define ENGINE_EFFECTS_GAMEOPTIONS_H_

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

enum class Character { Franko, Alex };

struct GameOptions {
  bool music = true;
  bool bass = false;
  bool mono = false;
  bool ntsc = false;
  bool tallScreen = false;
  Character character = Character::Franko;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_GAMEOPTIONS_H_
