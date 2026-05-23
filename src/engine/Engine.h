#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include "../player/Player.h"
#include "../systems/AudioSystem.h"
#include "../systems/ControllerSystem.h"
#include "../systems/VideoSystem.h"

namespace openfranko {
namespace src {
namespace engine {

class Engine {
public:
  Engine();
  ~Engine();

  bool isRunning();

  void update();

private:
  SDL_Event event;
  bool running;
  systems::VideoSystem videoSystem;
  systems::AudioSystem audioSystem;
  systems::ControllerSystem controllerSystem;

  player::Player player;
};

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ENGINE_H_