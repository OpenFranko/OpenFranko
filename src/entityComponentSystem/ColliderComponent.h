#ifndef ENTITYCOMPONENTSYSTEM_COLLIDERCOMPONENT_COLLIDERCOMPONENT_H_
#define ENTITYCOMPONENTSYSTEM_COLLIDERCOMPONENT_COLLIDERCOMPONENT_H_

#include "../game/Game.h"
#include "EntityComponentSystem.h"
#include "SpriteComponent.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>

namespace openfranko {
namespace src {
namespace entityComponentSystem {

class ColliderComponent : public Component {
public:
  SDL_Rect collider;
  std::string tag;

  SpriteComponent *sprite;

  ColliderComponent(const std::string &t) { tag = t; }

  void init() override {
    if (!entity->hasComponent<SpriteComponent>()) {
      entity->addComponent<SpriteComponent>();
    }
    sprite = &entity->getComponent<SpriteComponent>();

    openfranko::src::game::Game::colliders.push_back(this);
  }

  void update() override {
    collider.x = static_cast<int>(sprite->spriteRect.x);
    collider.y = static_cast<int>(sprite->spriteRect.y);
    collider.w = sprite->spriteRect.w;
    collider.h = sprite->spriteRect.h;
  }
};

} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_COLLIDERCOMPONENT_COLLIDERCOMPONENT_H_