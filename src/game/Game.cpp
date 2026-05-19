#include "Game.h"
#include "../collision/Collision.h"
#include "../entityComponentSystem/Components.h"
#include "../vector2d/Vector2D.h"
#include <iostream>

namespace openfranko::src::game {

entityComponentSystem::Manager manager;

SDL_Renderer *Game::renderer = nullptr;
SDL_Event Game::event;

std::vector<openfranko::src::entityComponentSystem::ColliderComponent *>
    Game::colliders;

auto &player(manager.addEntity());
auto &wall(manager.addEntity());

enum groupLabels : size_t {
  groupMap,
  groupPlayers,
  groupEnemies,
  groupColliders
};

Game::Game() {}
Game::~Game() {}

void Game::init(const char *title, int xpos, int ypos, int width, int height,
                bool fullscreen) {
  int flags = 0;
  if (fullscreen) {
    flags |= SDL_WINDOW_FULLSCREEN;
  }

  if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
    std::cout << "Subsystems Initialized!" << std::endl;
    m_window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
    if (m_window) {
      std::cout << "Window created!" << std::endl;
      renderer = SDL_CreateRenderer(m_window, -1, 0);
      if (renderer) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        std::cout << "Renderer created!" << std::endl;
      }
      m_running = true;
    }
  } else {
    m_running = false;
  }

  player.addComponent<entityComponentSystem::TransformComponent>(2);

  std::vector<std::string> idle = {"assets/00FF/00FF_006.bmp"};

  std::vector<std::string> walk = {
      "assets/00FF/00FF_000.bmp", "assets/00FF/00FF_001.bmp",
      "assets/00FF/00FF_002.bmp", "assets/00FF/00FF_003.bmp",
      "assets/00FF/00FF_004.bmp", "assets/00FF/00FF_005.bmp"};

  player.addComponent<entityComponentSystem::SpriteComponent>(
      std::map<std::string, std::vector<std::string>>{{"Idle", idle},
                                                      {"Walk", walk}});
  player.addComponent<entityComponentSystem::KeyboardController>();
  player.addComponent<entityComponentSystem::ColliderComponent>("player");
  player.addGroup(groupPlayers);
}

void Game::handleEvents() {

  SDL_PollEvent(&event);

  switch (event.type) {
  case SDL_QUIT:
    m_running = false;
    break;
  default:
    break;
  }
}

void Game::update() {
  manager.refresh();
  manager.update();

  for (auto cc : colliders) {
    collision::Collision::AABB(
        player.getComponent<entityComponentSystem::ColliderComponent>(), *cc);
  }
}

auto &tiles(manager.getGroup(groupMap));
auto &players(manager.getGroup(groupPlayers));
auto &enemies(manager.getGroup(groupEnemies));

void Game::render() {
  SDL_RenderClear(renderer);
  for (auto &t : tiles) {
    t->draw();
  }
  for (auto &p : players) {
    p->draw();
  }
  for (auto &e : enemies) {
    e->draw();
  }
  SDL_RenderPresent(renderer);
}

void Game::clean() {
  SDL_DestroyWindow(m_window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();
  std::cout << "Game cleaned!" << std::endl;
}

} // namespace openfranko::src::game