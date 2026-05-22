#include "engine/Engine.h"

using namespace openfranko::src;

int main() {

  engine::Engine engine;

  engine.videoSystem.createScreen(0, 320, 240);
  engine.videoSystem.switchScreen(0);

  const std::vector<std::string> walkFramePaths = {
      "assets/00FF/00FF_000.bmp", "assets/00FF/00FF_001.bmp",
      "assets/00FF/00FF_002.bmp", "assets/00FF/00FF_003.bmp",
      "assets/00FF/00FF_004.bmp", "assets/00FF/00FF_005.bmp"};

  engine.videoSystem.loadAnimation("idle", {"assets/00FF/00FF_006.bmp"});
  engine.videoSystem.loadAnimation("walk", walkFramePaths);

  engine.videoSystem.loadBackground("assets/0388.bmp");

  engine.audioSystem.loadMusic("assets/0259.s3m");
  engine.audioSystem.loadSFX("ready", "assets/00FF/00FF_sam12_6573Hz.wav");

  engine.audioSystem.playMusic();
  engine.audioSystem.playSFX("ready");

  int playerX = 150;
  int playerY = 180;
  int animFrame = 0;
  Uint32 lastTime = SDL_GetTicks();
  SDL_RendererFlip flipState = SDL_FLIP_NONE;

  std::string animState = "idle";
  int currentFrameCount = 1;

  while (engine.isRunning()) {
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

    engine.videoSystem.drawBackground();

    engine.videoSystem.drawAnimationFrame(animState, playerX, playerY,
                                          animFrame, flipState);

    engine.videoSystem.sync();
  }
  return 0;
}
