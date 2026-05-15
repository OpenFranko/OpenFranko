#include "TextureManager.h"
#include "../game/Game.h"

namespace openfranko::src::textureManager {

SDL_Texture *TextureManager::LoadTexture(const char *fileName) {
  SDL_Surface *tempSurface = IMG_Load(fileName);
  SDL_Texture *tex =
      SDL_CreateTextureFromSurface(game::Game::renderer, tempSurface);
  SDL_FreeSurface(tempSurface);
  return tex;
}

void TextureManager::draw(SDL_Texture *tex, SDL_Rect src, SDL_Rect dest) {
  SDL_RenderCopy(game::Game::renderer, tex, &src, &dest);
}

} // namespace openfranko::src::textureManager
