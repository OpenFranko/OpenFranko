#ifndef ENTITYCOMPONENTSYSTEM_COLLIDERCOMPONENT_COLLIDERCOMPONENT_H_
#define ENTITYCOMPONENTSYSTEM_COLLIDERCOMPONENT_COLLIDERCOMPONENT_H_

#include "../game/Game.h"
#include "EntityComponentSystem.h"
#include "TransformComponent.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>

namespace openfranko {
namespace src {
namespace entityComponentSystem {

class ColliderComponent : public entityComponentSystem::Component {
public:
  SDL_Rect collider;
  std::string tag;

  TransformComponent *transform;

  ColliderComponent(const std::string &t) { tag = t; }

  void init() override {
    if (!entity->hasComponent<TransformComponent>()) {
      entity->addComponent<TransformComponent>();
    }
    transform = &entity->getComponent<TransformComponent>();

    openfranko::src::game::Game::colliders.push_back(this);
  }

  void update() override {
    collider.x = static_cast<int>(transform->position.x);
    collider.y = static_cast<int>(transform->position.y);
    collider.w = transform->width * transform->scale;
    collider.h = transform->height * transform->scale;
  }
};

} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_COLLIDERCOMPONENT_COLLIDERCOMPONENT_H_