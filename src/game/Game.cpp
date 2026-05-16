#include "Game.h"
#include "../components/Components.h"
#include "../entityComponentSystem/EntityComponentSystem.h"
#include "../gameObject/GameObject.h"
#include "../map/Map.h"
#include <iostream>

namespace openfranko::src::game {

gameObject::GameObject *player;
gameObject::GameObject *enemy;
map::Map *map;

SDL_Renderer *Game::renderer = nullptr;

entityComponentSystem::Manager manager;
auto &newPlayer(manager.addEntity());

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

  player = new gameObject::GameObject("assets/00FF/00FF_006.bmp", 0, 0);
  enemy = new gameObject::GameObject("assets/0038/0038_009.bmp", 100, 50);
  map = new map::Map();

  newPlayer.addComponent<components::PositionComponent>();
  newPlayer.getComponent<components::PositionComponent>().setPos(500, 500);
}

void Game::handleEvents() {
  SDL_Event event;
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
  player->update();
  enemy->update();
  manager.update();
  std::cout << newPlayer.getComponent<components::PositionComponent>().x()
            << ","
            << newPlayer.getComponent<components::PositionComponent>().y()
            << std::endl;
}

void Game::render() {
  SDL_RenderClear(renderer);
  map->drawMap();
  player->render();
  enemy->render();
  SDL_RenderPresent(renderer);
}

void Game::clean() {
  SDL_DestroyWindow(m_window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();
  std::cout << "Game cleaned!" << std::endl;
}

} // namespace openfranko::src::game