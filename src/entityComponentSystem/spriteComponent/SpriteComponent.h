#ifndef ENTITYCOMPONENTSYSTEM_SPRITECOMPONENT_SPRITECOMPONENT_H_
#define ENTITYCOMPONENTSYSTEM_SPRITECOMPONENT_SPRITECOMPONENT_H_

#include "../../textureManager/TextureManager.h"
#include "../EntityComponentSystem.h"
#include "../positionComponent/PositionComponent.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace openfranko {
namespace src {
namespace entityComponentSystem {
namespace spriteComponent {

class SpriteComponent : public Component {
private:
  positionComponent::PositionComponent *position;
  SDL_Texture *texture;
  SDL_Rect srcRect;
  SDL_Rect destRect;

public:
  SpriteComponent() = default;
  SpriteComponent(const char *path) { setTex(path); }

  void setTex(const char *path) {
    texture = textureManager::TextureManager::LoadTexture(path);
  }

  void init() override {
    position = &entity->getComponent<positionComponent::PositionComponent>();

    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = 32;
    srcRect.h = 79;
    destRect.w = srcRect.w * 2;
    destRect.h = srcRect.h * 2;
  }

  void update() override {
    destRect.x = position->x();
    destRect.y = position->y();
  }

  void draw() override {
    textureManager::TextureManager::draw(texture, srcRect, destRect);
  }
};

} // namespace spriteComponent
} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_SPRITECOMPONENT_SPRITECOMPONENT_H_