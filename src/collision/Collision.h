#ifndef COLLISION_COLLISION_H_
#define COLLISION_COLLISION_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace openfranko {
namespace src {
namespace collision {

class Collision {
public:
  static bool AABB(const SDL_Rect &rectA, const SDL_Rect &rectB);
};

} // namespace collision
} // namespace src
} // namespace openfranko

#endif // COLLISION_COLLISION_H_