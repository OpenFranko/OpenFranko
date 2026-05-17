#ifndef ENTITYCOMPONENTSYSTEM_TILECOMPONENT_TILECOMPONENT_H_
#define ENTITYCOMPONENTSYSTEM_TILECOMPONENT_TILECOMPONENT_H_

#include "../EntityComponentSystem.h"
#include "../spriteComponent/SpriteComponent.h"
#include "../transformComponent/TransformComponent.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace openfranko {
namespace src {
namespace entityComponentSystem {
namespace tileComponent {

class TileComponent : public Component {
public:
  transformComponent::TransformComponent *transform;
  spriteComponent::SpriteComponent *sprite;

  SDL_Rect tileRect;
  int tileID;
  char *path;

  TileComponent() = default;

  TileComponent(int x, int y, int w, int h, int id) {
    tileRect.x = x;
    tileRect.y = y;
    tileRect.w = w;
    tileRect.h = h;
    tileID = id;

    switch (tileID) {
    case 0:
      path = "assets/0035/0035_003.bmp";
      break;
    case 1:
      path = "assets/0035/0035_004.bmp";
      break;
    case 2:
      path = "assets/0035/0035_005.bmp";
      break;
    default:
      break;
    }
  }

  void init() override {
    entity->addComponent<transformComponent::TransformComponent>(
        (float)tileRect.x, (float)tileRect.y, tileRect.w, tileRect.h, 1);
    transform = &entity->getComponent<transformComponent::TransformComponent>();

    entity->addComponent<spriteComponent::SpriteComponent>(path);
    sprite = &entity->getComponent<spriteComponent::SpriteComponent>();
  }
};

} // namespace tileComponent
} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_TILECOMPONENT_TILECOMPONENT_H_