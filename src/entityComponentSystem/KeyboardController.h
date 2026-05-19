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
    const Uint8 *keystate = SDL_GetKeyboardState(nullptr);

    transform->velocity.x = 0;
    transform->velocity.y = 0;
    bool isMoving = false;

    if (keystate[SDL_SCANCODE_A]) {
      transform->velocity.x = -1;
      sprite->spriteFlip = SDL_FLIP_HORIZONTAL;
      isMoving = true;
    } else if (keystate[SDL_SCANCODE_D]) {
      transform->velocity.x = 1;
      sprite->spriteFlip = SDL_FLIP_NONE;
      isMoving = true;
    }

    if (keystate[SDL_SCANCODE_W]) {
      transform->velocity.y = -1;
      isMoving = true;
    } else if (keystate[SDL_SCANCODE_S]) {
      transform->velocity.y = 1;
      isMoving = true;
    }

    if (isMoving) {
      sprite->play("Walk");
    } else {
      sprite->play("Idle");
    }
  }
};

} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_KEYBOARDCONTROLLER_KEYBOARDCONTROLLER_H_