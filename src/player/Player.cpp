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

constexpr auto ANIMATION_DELAY = 120;

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
  animState = "idle";
  currentFrameCount = idleFramePaths.size();

  if (buttons.up) {
    y--;
    animState = "walk";
    currentFrameCount = walkFramePaths.size();
  }
  if (buttons.down) {
    y++;
    animState = "walk";
    currentFrameCount = walkFramePaths.size();
  }
  if (buttons.left) {
    x--;
    animState = "walk";
    currentFrameCount = walkFramePaths.size();
    flipState = SDL_FLIP_HORIZONTAL;
  }
  if (buttons.right) {
    x++;
    animState = "walk";
    currentFrameCount = walkFramePaths.size();
    flipState = SDL_FLIP_NONE;
  }
}

void Player::animate() {
  if (SDL_GetTicks() - lastTime > ANIMATION_DELAY) {
    animFrame++;
    lastTime = SDL_GetTicks();
  }

  if (animFrame >= currentFrameCount) {
    animFrame = 0;
  }
}

} // namespace openfranko::src::player