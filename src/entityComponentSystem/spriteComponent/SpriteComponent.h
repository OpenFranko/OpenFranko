#ifndef ENTITYCOMPONENTSYSTEM_SPRITECOMPONENT_SPRITECOMPONENT_H_
#define ENTITYCOMPONENTSYSTEM_SPRITECOMPONENT_SPRITECOMPONENT_H_

#include "../../textureManager/TextureManager.h"
#include "../EntityComponentSystem.h"
#include "../transformComponent/TransformComponent.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace openfranko {
namespace src {
namespace entityComponentSystem {
namespace spriteComponent {

class SpriteComponent : public Component {
private:
  transformComponent::TransformComponent *transform;
  int frame = 0;
  std::vector<SDL_Texture *> textures;
  SDL_Rect srcRect;
  SDL_Rect destRect;

  bool animated = false;
  int frames = 0;
  int speed = 100;

public:
  SpriteComponent() = default;
  SpriteComponent(const char *path) { setTex(path); }
  SpriteComponent(const std::vector<std::string> &paths, int mSpeed) {
    animated = true;
    frames = paths.size();
    speed = mSpeed;
    setTexs(paths);
  }
  ~SpriteComponent() {
    for (auto texture : textures) {
      SDL_DestroyTexture(texture);
    }
  }

  void setTex(const char *path) {
    textures.emplace_back(textureManager::TextureManager::LoadTexture(path));
  }

  void setTexs(const std::vector<std::string> &paths) {
    for (auto &path : paths) {
      textures.emplace_back(
          textureManager::TextureManager::LoadTexture(path.c_str()));
    }
  }

  void init() override {
    transform = &entity->getComponent<transformComponent::TransformComponent>();

    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = transform->width;
    srcRect.h = transform->height;
  }

  void update() override {
    if (animated) {
      frame = static_cast<int>((SDL_GetTicks() / speed) % frames);
    }

    destRect.x = static_cast<int>(transform->position.x);
    destRect.y = static_cast<int>(transform->position.y);
    destRect.w = transform->width * transform->scale;
    destRect.h = transform->height * transform->scale;
  }

  void draw() override {
    textureManager::TextureManager::draw(textures.at(frame), srcRect, destRect);
  }
};

} // namespace spriteComponent
} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_SPRITECOMPONENT_SPRITECOMPONENT_H_