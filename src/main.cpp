#include "engine/Engine.h"

using namespace openfranko::src;

int main() {

  engine::Engine engine;

  if (!engine.init("OpenFranko", 800, 600)) {
    return -1;
  }

  engine.screenOpen(0, 320, 240);

  const std::vector<std::string> walkFramePaths = {
      "assets/00FF/00FF_000.bmp", "assets/00FF/00FF_001.bmp",
      "assets/00FF/00FF_002.bmp", "assets/00FF/00FF_003.bmp",
      "assets/00FF/00FF_004.bmp", "assets/00FF/00FF_005.bmp"};

  engine.loadAnimation("idle", {"assets/00FF/00FF_006.bmp"});
  engine.loadAnimation("walk", walkFramePaths);

  int playerX = 100;
  int playerY = 120;
  int animFrame = 0;
  Uint32 lastTime = SDL_GetTicks();
  SDL_RendererFlip flipState = SDL_FLIP_NONE;

  std::string animState = "idle";
  int currentFrameCount = 1;

  while (engine.loop()) {
    engine.cls(10, 20, 40);

    const Uint8 *keys = SDL_GetKeyboardState(nullptr);
    bool moving = false;

    if (keys[SDL_SCANCODE_W]) {
      playerY--;
      moving = true;
    }
    if (keys[SDL_SCANCODE_S]) {
      playerY++;
      moving = true;
    }
    if (keys[SDL_SCANCODE_A]) {
      playerX--;
      moving = true;
      flipState = SDL_FLIP_HORIZONTAL;
    }
    if (keys[SDL_SCANCODE_D]) {
      playerX++;
      moving = true;
      flipState = SDL_FLIP_NONE;
    }

    if (moving) {
      animState = "walk";
      currentFrameCount = walkFramePaths.size();
    } else {
      animState = "idle";
      currentFrameCount = 1;
    }

    if (moving && SDL_GetTicks() - lastTime > 100) {
      animFrame++;
      if (animFrame >= currentFrameCount) {
        animFrame = 0;
      }
      lastTime = SDL_GetTicks();
    } else if (!moving) {
      animFrame = 0;
    }

    engine.sprite(animState, playerX, playerY, animFrame, flipState);

    engine.sync();
  }
  return 0;
}
