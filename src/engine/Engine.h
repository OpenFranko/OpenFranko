#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include "../systems/AudioSystem.h"
#include "../systems/VideoSystem.h"
#include <SDL2/SDL_mixer.h>
#include <string>

namespace openfranko {
namespace src {
namespace engine {

class Engine {
public:
  static SDL_Event event;

  Engine();
  ~Engine();

  bool isRunning();

  systems::VideoSystem videoSystem;
  systems::AudioSystem audioSystem;

private:
  bool running;
};

inline SDL_Event Engine::event;

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ENGINE_H_