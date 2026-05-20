#ifndef TEXTUREMANAGER_TEXTUREMANAGER_H_
#define TEXTUREMANAGER_TEXTUREMANAGER_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace openfranko {
namespace src {
namespace textureManager {

class TextureManager {
public:
  static std::pair<SDL_Texture *, SDL_Surface *>
  LoadTexture(const char *fileName);
  static void draw(SDL_Texture *tex, SDL_Rect src, SDL_Rect dest,
                   SDL_RendererFlip flip);
};

} // namespace textureManager
} // namespace src
} // namespace openfranko

#endif // TEXTUREMANAGER_TEXTUREMANAGER_H_