#ifndef PLAYER_PLAYER_H_
#define PLAYER_PLAYER_H_

#include "../systems/ControllerSystem.h"
#include "../systems/VideoSystem.h"

namespace openfranko {
namespace src {
namespace player {

class Player {
public:
  Player(systems::VideoSystem &videoSystem,
         systems::ControllerSystem &controllerSystem);

  void update();

  void draw();

private:
  void move();
  void animate();

  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;

  int x = 150;
  int y = 180;
  int animFrame = 0;
  SDL_RendererFlip flipState = SDL_FLIP_NONE;
  uint32_t lastTime = 0;

  std::string animState = "idle";
  int currentFrameCount = 1;
};

} // namespace player
} // namespace src
} // namespace openfranko

#endif // PLAYER_PLAYER_H_