#ifndef TEXTUREMANAGER_TEXTUREMANAGER_H_
#define TEXTUREMANAGER_TEXTUREMANAGER_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace openfranko {
namespace src {
namespace textureManager {

class TextureManager {
public:
  static SDL_Texture *LoadTexture(const char *fileName, SDL_Renderer *renderer);
};

} // namespace textureManager
} // namespace src
} // namespace openfranko

#endif // TEXTUREMANAGER_TEXTUREMANAGER_H_