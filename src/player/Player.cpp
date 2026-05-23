#include "Player.h"
#include <string>
#include <vector>

namespace openfranko::src::player {
namespace {

const std::vector<std::string> walkFramePaths = {
    "assets/00FF/00FF_000.bmp", "assets/00FF/00FF_001.bmp",
    "assets/00FF/00FF_002.bmp", "assets/00FF/00FF_003.bmp",
    "assets/00FF/00FF_004.bmp", "assets/00FF/00FF_005.bmp"};

const std::vector<std::string> idleFramePaths = {"assets/00FF/00FF_006.bmp"};

} // namespace

Player::Player(systems::VideoSystem &videoSystem,
               systems::ControllerSystem &controllerSystem)
    : m_videoSystem{videoSystem}, m_controllerSystem{controllerSystem} {

  m_videoSystem.loadAnimation("walk", walkFramePaths);
  m_videoSystem.loadAnimation("idle", idleFramePaths);
}

void Player::update() {
  move();
  animate();
}

void Player::draw() {
  m_videoSystem.drawAnimationFrame(animState, x, y, animFrame, flipState);
}

void Player::move() {
  auto buttons = m_controllerSystem.states;
  moving = false;

  if (buttons.up) {
    y--;
    moving = true;
  }
  if (buttons.down) {
    y++;
    moving = true;
  }
  if (buttons.left) {
    x--;
    moving = true;
    flipState = SDL_FLIP_HORIZONTAL;
  }
  if (buttons.right) {
    x++;
    moving = true;
    flipState = SDL_FLIP_NONE;
  }
}

void Player::animate() {
  if (moving) {
    animState = "walk";
    currentFrameCount = walkFramePaths.size();
  } else {
    animState = "idle";
    currentFrameCount = 1;
  }

  if (moving && SDL_GetTicks() - lastTime > 120) {
    animFrame++;
    if (animFrame >= currentFrameCount) {
      animFrame = 0;
    }
    lastTime = SDL_GetTicks();
  } else if (!moving) {
    animFrame = 0;
  }
}

} // namespace openfranko::src::player