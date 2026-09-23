#ifndef ENGINE_EFFECTS_GAMEOPTIONS_H_
#define ENGINE_EFFECTS_GAMEOPTIONS_H_

namespace openfranko {
namespace src {
namespace engine {
namespace effects {

struct GameOptions {
  bool music = false;
  bool bass = false;
  bool mono = false;
  bool ntsc = false;
  bool tallScreen = false;
};

} // namespace effects
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_EFFECTS_GAMEOPTIONS_H_
