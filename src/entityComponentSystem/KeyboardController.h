#ifndef ENTITYCOMPONENTSYSTEM_KEYBOARDCONTROLLER_KEYBOARDCONTROLLER_H_
#define ENTITYCOMPONENTSYSTEM_KEYBOARDCONTROLLER_KEYBOARDCONTROLLER_H_

#include "../game/Game.h"
#include "EntityComponentSystem.h"
#include "SpriteComponent.h"
#include "TransformComponent.h"

namespace openfranko {
namespace src {
namespace entityComponentSystem {

class KeyboardController : public Component {
public:
  TransformComponent *transform;
  SpriteComponent *sprite;

  void init() override {
    transform = &entity->getComponent<TransformComponent>();
    sprite = &entity->getComponent<SpriteComponent>();
  }

  void update() override {
    if (game::Game::event.type == SDL_KEYDOWN) {
      switch (game::Game::event.key.keysym.sym) {
      case SDLK_a:
        transform->velocity.x = -1;
        sprite->play("Walk");
        sprite->spriteFlip = SDL_FLIP_HORIZONTAL;
        break;

      case SDLK_d:
        transform->velocity.x = 1;
        sprite->play("Walk");
        sprite->spriteFlip = SDL_FLIP_NONE;
        break;

      case SDLK_w:
        transform->velocity.y = -1;
        sprite->play("Walk");
        break;

      case SDLK_s:
        transform->velocity.y = 1;
        sprite->play("Walk");
        break;

      default:
        break;
      }
    }

    if (game::Game::event.type == SDL_KEYUP) {
      switch (game::Game::event.key.keysym.sym) {
      case SDLK_a:
        transform->velocity.x = 0;
        sprite->play("Idle");
        break;

      case SDLK_d:
        transform->velocity.x = 0;
        sprite->play("Idle");
        break;

      case SDLK_w:
        transform->velocity.y = 0;
        sprite->play("Idle");
        break;

      case SDLK_s:
        transform->velocity.y = 0;
        sprite->play("Idle");
        break;

      default:
        break;
      }
    }
  }
};

} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_KEYBOARDCONTROLLER_KEYBOARDCONTROLLER_H_