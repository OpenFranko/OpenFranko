#ifndef GAME_GAME_H_
#define GAME_GAME_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>

namespace openfranko {
namespace src {
namespace entityComponentSystem {
namespace colliderComponent {

class ColliderComponent;

}
} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

namespace openfranko {
namespace src {
namespace game {

class Game {
public:
  Game();
  ~Game();

  void init(const char *title, int xpos, int ypos, int width, int height,
            bool fullscreen);

  void handleEvents();
  void update();
  void render();
  void clean();

  bool running() { return m_running; }

  static void addTile(int id, int x, int y);
  static SDL_Renderer *renderer;
  static SDL_Event event;
  static std::vector<openfranko::src::entityComponentSystem::colliderComponent::
                         ColliderComponent *>
      colliders;

private:
  int m_count = 0;

  bool m_running;
  SDL_Window *m_window;
};

} // namespace game
} // namespace src
} // namespace openfranko

#endif // GAME_GAME_H_