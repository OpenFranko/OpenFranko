#include "Collision.h"
#include "../entityComponentSystem/ColliderComponent.h"

namespace openfranko::src::collision {

bool Collision::AABB(const SDL_Rect &rectA, const SDL_Rect &rectB) {
  return (rectA.x + rectA.w >= rectB.x && rectB.x + rectB.w >= rectA.x &&
          rectA.y + rectA.h >= rectB.y && rectB.y + rectB.h >= rectA.y);
}

bool Collision::AABB(
    const openfranko::src::entityComponentSystem::ColliderComponent &colA,
    const openfranko::src::entityComponentSystem::ColliderComponent &colB) {
  return AABB(colA.collider, colB.collider);
}

} // namespace openfranko::src::collision