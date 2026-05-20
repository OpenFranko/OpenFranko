#ifndef ENTITYCOMPONENTSYSTEM_TRANSFORMCOMPONENT_TRANSFORMCOMPONENT_H_
#define ENTITYCOMPONENTSYSTEM_TRANSFORMCOMPONENT_TRANSFORMCOMPONENT_H_

#include "../vector2d/Vector2D.h"
#include "EntityComponentSystem.h"

namespace openfranko {
namespace src {
namespace entityComponentSystem {

class TransformComponent : public entityComponentSystem::Component {
public:
  vector2d::Vector2D position;
  vector2d::Vector2D velocity;

  int speed = 3;

  TransformComponent() { position.zero(); }

  TransformComponent(float x, float y) {
    position.x = x;
    position.y = y;
  }

  void init() override { velocity.zero(); }
  void update() override {
    position.x += velocity.x * speed;
    position.y += velocity.y * speed;
  }
};

} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_TRANSFORMCOMPONENT_TRANSFORMCOMPONENT_H_