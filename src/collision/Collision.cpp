#include "Collision.h"
#include "../entityComponentSystem/ColliderComponent.h"

namespace openfranko::src::collision {

namespace {
uint8_t GetPixelAlpha(SDL_Surface *surface, int x, int y) {
  if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
    return 0;
  }
  uint32_t *pixels = static_cast<uint32_t *>(surface->pixels);
  uint32_t pixelToken = pixels[(y * (surface->pitch / 4)) + x];
  uint8_t r, g, b, a;
  SDL_GetRGBA(pixelToken, surface->format, &r, &g, &b, &a);

  return a;
}
} // namespace

bool Collision::AABB(const SDL_Rect &rectA, const SDL_Rect &rectB) {
  return (rectA.x + rectA.w >= rectB.x && rectB.x + rectB.w >= rectA.x &&
          rectA.y + rectA.h >= rectB.y && rectB.y + rectB.h >= rectA.y);
}

bool Collision::AABB(
    const openfranko::src::entityComponentSystem::ColliderComponent &colA,
    const openfranko::src::entityComponentSystem::ColliderComponent &colB) {
  return AABB(colA.collider, colB.collider);
}

bool Collision::CheckPixelCollision(SDL_Surface *surfaceA, int xA, int yA,
                                    SDL_Surface *surfaceB, int xB, int yB) {

  SDL_Rect boxA = {xA, yA, surfaceA->w, surfaceA->h};
  SDL_Rect boxB = {xB, yB, surfaceB->w, surfaceB->h};

  SDL_Rect intersection;
  if (!SDL_IntersectRect(&boxA, &boxB, &intersection)) {
    return false;
  }

  if (SDL_MUSTLOCK(surfaceA))
    SDL_LockSurface(surfaceA);
  if (SDL_MUSTLOCK(surfaceB))
    SDL_LockSurface(surfaceB);

  for (int y = intersection.y; y < intersection.y + intersection.h; ++y) {
    for (int x = intersection.x; x < intersection.x + intersection.w; ++x) {

      int localX_A = x - xA;
      int localY_A = y - yA;

      int localX_B = x - xB;
      int localY_B = y - yB;

      uint8_t alphaA = GetPixelAlpha(surfaceA, localX_A, localY_A);
      uint8_t alphaB = GetPixelAlpha(surfaceB, localX_B, localY_B);

      if (alphaA > 0 && alphaB > 0) {
        if (SDL_MUSTLOCK(surfaceA))
          SDL_UnlockSurface(surfaceA);
        if (SDL_MUSTLOCK(surfaceB))
          SDL_UnlockSurface(surfaceB);
        return true;
      }
    }
  }

  if (SDL_MUSTLOCK(surfaceA))
    SDL_UnlockSurface(surfaceA);
  if (SDL_MUSTLOCK(surfaceB))
    SDL_UnlockSurface(surfaceB);

  return false;
}

} // namespace openfranko::src::collision