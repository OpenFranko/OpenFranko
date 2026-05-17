#ifndef ENTITYCOMPONENTSYSTEM_TRANSFORMCOMPONENT_TRANSFORMCOMPONENT_H_
#define ENTITYCOMPONENTSYSTEM_TRANSFORMCOMPONENT_TRANSFORMCOMPONENT_H_

#include "../../vector2d/Vector2D.h"
#include "../EntityComponentSystem.h"

namespace openfranko {
namespace src {
namespace entityComponentSystem {
namespace transformComponent {

class TransformComponent : public entityComponentSystem::Component {
public:
  vector2d::Vector2D position;

  TransformComponent() {
    position.x = 0.0f;
    position.y = 0.0f;
  }

  TransformComponent(float x, float y) {
    position.x = x;
    position.y = y;
  }

  void update() override {}
};

} // namespace transformComponent
} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

#endif // ENTITYCOMPONENTSYSTEM_TRANSFORMCOMPONENT_TRANSFORMCOMPONENT_H_