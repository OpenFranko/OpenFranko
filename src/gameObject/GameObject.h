#ifndef GAMEOBJECT_GAMEOBJECT_CPP_
#define GAMEOBJECT_GAMEOBJECT_CPP_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace openfranko {
namespace src {
namespace gameObject {

class GameObject {
public:
  GameObject(const char *textureSheet, SDL_Renderer *renderer, int x, int y);
  ~GameObject();

  void update();
  void render();

private:
  int m_xpos;
  int m_ypos;

  SDL_Texture *m_objTexture;
  SDL_Rect m_srcRect, m_destRect;
  SDL_Renderer *m_renderer;
};

} // namespace gameObject
} // namespace src
} // namespace openfranko

#endif // GAMEOBJECT_GAMEOBJECT_CPP_