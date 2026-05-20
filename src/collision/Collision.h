#ifndef COLLISION_COLLISION_H_
#define COLLISION_COLLISION_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace openfranko {
namespace src {
namespace entityComponentSystem {

class ColliderComponent;

} // namespace entityComponentSystem
} // namespace src
} // namespace openfranko

namespace openfranko {
namespace src {
namespace collision {

class Collision {
public:
  static bool AABB(const SDL_Rect &rectA, const SDL_Rect &rectB);
  static bool
  AABB(const openfranko::src::entityComponentSystem::ColliderComponent &colA,
       const openfranko::src::entityComponentSystem::ColliderComponent &colB);

  static bool CheckPixelCollision(SDL_Surface *surfaceA, int xA, int yA,
                                  SDL_Surface *surfaceB, int xB, int yB);
};

} // namespace collision
} // namespace src
} // namespace openfranko

#endif // COLLISION_COLLISION_H_