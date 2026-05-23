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
  setAnimationState("idle");

  if (buttons.up) {
    y--;
    setAnimationState("walk");
  }
  if (buttons.down) {
    y++;
    setAnimationState("walk");
  }
  if (buttons.left) {
    x--;
    setAnimationState("walk");
    flipState = SDL_FLIP_HORIZONTAL;
  }
  if (buttons.right) {
    x++;
    setAnimationState("walk");
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

void Player::setAnimationState(const std::string &name) {
  animState = name;
  currentFrameCount = m_videoSystem.getAnimationSize(name);
}

} // namespace openfranko::src::player