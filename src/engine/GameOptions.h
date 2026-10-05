#ifndef ENGINE_GAMEOPTIONS_H_
#define ENGINE_GAMEOPTIONS_H_

namespace openfranko {
namespace src {
namespace engine {

enum class Character { Franko, Alex };

struct GameOptions {
  bool music = true;
  bool bass = false;
  bool mono = false;
#if defined(__DJGPP__) || defined(DJGPP)
  bool ntsc = true;
#else
  bool ntsc = false;
#endif
  bool tallScreen = false;
  Character character = Character::Franko;
};

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_GAMEOPTIONS_H_
