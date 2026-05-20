#include "TextureManager.h"
#include "../game/Game.h"
#include <iostream>

namespace openfranko::src::textureManager {

std::pair<SDL_Texture *, SDL_Surface *>
TextureManager::LoadTexture(const char *fileName) {
  SDL_Surface *surface = IMG_Load(fileName);
  Uint32 colorKey = SDL_MapRGB(surface->format, 85, 85, 85);

  if (SDL_SetColorKey(surface, SDL_TRUE, colorKey) < 0) {
    std::cerr << "Unable to set color key! SDL Error: " << SDL_GetError()
              << std::endl;
  }

  SDL_Texture *tex =
      SDL_CreateTextureFromSurface(game::Game::renderer, surface);
  return std::make_pair(tex, surface);
}

void TextureManager::draw(SDL_Texture *tex, SDL_Rect src, SDL_Rect dest,
                          SDL_RendererFlip flip) {
  SDL_RenderCopyEx(game::Game::renderer, tex, &src, &dest, NULL, NULL, flip);
}

} // namespace openfranko::src::textureManager
