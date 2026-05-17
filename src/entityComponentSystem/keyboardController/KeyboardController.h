#ifndef ENTITYCOMPONENTSYSTEM_KEYBOARDCONTROLLER_KEYBOARDCONTROLLER_H_
#define ENTITYCOMPONENTSYSTEM_KEYBOARDCONTROLLER_KEYBOARDCONTROLLER_H_

#include "../../game/Game.h"
#include "../EntityComponentSystem.h"
#include "../transformComponent/TransformComponent.h"

namespace openfranko {
namespace src {
namespace entityComponentSystem {
namespace keyboardController {

class KeyboardController : public Component {
public:
  transformComponent::TransformComponent *transform;

  void init() override {
    transform = &entity->getComponent<transformComponent::TransformComponent>();
  }

  void update() override {
    if (game::Game::event.type == SDL_KEYDOWN) {
      switch (game::Game::event.key.keysym.sym) {
      case SDLK_a:
        transform->velocity.x = -1;
        break;

      case SDLK_d:
        transform->velocity.x = 1;
        break;

      case SDLK_w:
        transform->velocity.y = -1;
        break;

      case SDLK_s:
        transform->velocity.y = 1;
        break;

      default:
        break;
      }
    }

    if (game::Game::event.type == SDL_KEYUP) {
      switch (game::Game::event.key.keysym.sym) {
      case SDLK_a:
        transform->velocity.x = 0;
        break;

      case SDLK_d:
        transform->velocity.x = 0;
        break;

      case SDLK_w:
        transform->velocity.y = 0;
        break;

      case SDLK_s:
        transform->velocity.y = 0;
        break;

      default:
        break;
      }
    }
  }
};

} // namespace keyboardController
} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_KEYBOARDCONTROLLER_KEYBOARDCONTROLLER_H_