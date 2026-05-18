#include "Game.h"
#include "../collision/Collision.h"
#include "../entityComponentSystem/components/Components.h"
#include "../map/Map.h"
#include "../vector2d/Vector2D.h"
#include <iostream>

namespace openfranko::src::game {

map::Map *map;
entityComponentSystem::Manager manager;

SDL_Renderer *Game::renderer = nullptr;
SDL_Event Game::event;

std::vector<openfranko::src::entityComponentSystem::colliderComponent::
                ColliderComponent *>
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

  map = new map::Map();

  map::Map::loadMap("assets/p16x16.map", 16, 16);

  player.addComponent<
      entityComponentSystem::transformComponent::TransformComponent>(2);
  player.addComponent<entityComponentSystem::spriteComponent::SpriteComponent>(
      "assets/00FF/00FF_006.bmp");
  player.addComponent<
      entityComponentSystem::keyboardController::KeyboardController>();
  player.addComponent<
      entityComponentSystem::colliderComponent::ColliderComponent>("player");
  player.addGroup(groupPlayers);

  wall.addComponent<
      entityComponentSystem::transformComponent::TransformComponent>(
      300.0f, 300.0f, 300, 20, 1);
  wall.addComponent<entityComponentSystem::spriteComponent::SpriteComponent>(
      "assets/038A.bmp");
  wall.addComponent<
      entityComponentSystem::colliderComponent::ColliderComponent>("wall");
  wall.addGroup(groupMap);
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
        player.getComponent<
            entityComponentSystem::colliderComponent::ColliderComponent>(),
        *cc);
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

void Game::addTile(int id, int x, int y) {
  auto &tile(manager.addEntity());
  tile.addComponent<entityComponentSystem::tileComponent::TileComponent>(
      x, y, 32, 32, id);
  tile.addGroup(groupMap);
}

} // namespace openfranko::src::game