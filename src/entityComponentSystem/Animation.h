#ifndef ENTITYCOMPONENTSYSTEM_ANIMATION_ANIMATION_H_
#define ENTITYCOMPONENTSYSTEM_ANIMATION_ANIMATION_H_

#include "../textureManager/TextureManager.h"
#include <string>
#include <vector>

namespace openfranko {
namespace src {
namespace entityComponentSystem {

struct Animation {
  int speed;
  std::vector<SDL_Texture *> frameTexs;

  Animation() {}
  Animation(int s, const std::vector<std::string> &paths) {
    speed = s;
    for (auto &path : paths) {
      frameTexs.emplace_back(
          textureManager::TextureManager::LoadTexture(path.c_str()));
    }
  }
};

} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_ANIMATION_ANIMATION_H_
