#ifndef ENTITYCOMPONENTSYSTEM_SPRITECOMPONENT_SPRITECOMPONENT_H_
#define ENTITYCOMPONENTSYSTEM_SPRITECOMPONENT_SPRITECOMPONENT_H_

#include "../textureManager/TextureManager.h"
#include "Animation.h"
#include "EntityComponentSystem.h"
#include "TransformComponent.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <map>

namespace openfranko {
namespace src {
namespace entityComponentSystem {

class SpriteComponent : public Component {
private:
  TransformComponent *transform;
  int frame = 0;
  std::map<std::string, Animation> animations;

  SDL_Rect srcRect;
  SDL_Rect destRect;

  int frames = 0;
  int speed = 100;
  std::string animState = "Idle";

public:
  SDL_RendererFlip spriteFlip = SDL_FLIP_NONE;

  SpriteComponent() = default;
  SpriteComponent(const char *path) { setTex(path); }
  SpriteComponent(
      const std::map<std::string, std::vector<std::string>> &paths) {
    setTexs(paths);
  }
  ~SpriteComponent() {
    for (auto &animation : animations) {
      for (auto texture : animation.second.frameTexs) {
        SDL_DestroyTexture(texture);
      }
    }
  }

  void setTex(const char *path) {
    auto idle = Animation(speed, std::vector{std::string(path)});
    animations.emplace("Idle", idle);
  }

  void setTexs(const std::map<std::string, std::vector<std::string>> &pathMap) {
    for (auto &paths : pathMap) {
      animations.emplace(paths.first, Animation(speed, paths.second));
    }
  }

  void init() override {
    transform = &entity->getComponent<TransformComponent>();

    srcRect.x = 0;
    srcRect.y = 0;

    auto texture = animations.at(animState).frameTexs.at(frame);
    SDL_QueryTexture(texture, nullptr, nullptr, &transform->width,
                     &transform->height);

    srcRect.w = transform->width;
    srcRect.h = transform->height;
  }

  void update() override {
    auto animation = animations.at(animState);

    frame = static_cast<int>((SDL_GetTicks() / animation.speed) %
                             animation.frameTexs.size());

    auto texture = animation.frameTexs.at(frame);
    SDL_QueryTexture(texture, nullptr, nullptr, &transform->width,
                     &transform->height);

    if (spriteFlip == SDL_FLIP_HORIZONTAL) {
      destRect.x = static_cast<int>(transform->position.x - transform->width);
    } else {
      destRect.x = static_cast<int>(transform->position.x);
    }

    destRect.y = static_cast<int>(transform->position.y);

    srcRect.w = transform->width;
    srcRect.h = transform->height;
    destRect.w = transform->width * transform->scale;
    destRect.h = transform->height * transform->scale;
  }

  void draw() override {
    auto animation = animations.at(animState);
    textureManager::TextureManager::draw(animation.frameTexs.at(frame), srcRect,
                                         destRect, spriteFlip);
  }

  void play(const std::string &animName) {
    if (animState != animName) {
      frame = 0;
    }
    animState = animName;
  }
};

} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_SPRITECOMPONENT_SPRITECOMPONENT_H_