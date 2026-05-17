#include "Game.h"
#include "../entityComponentSystem/components/Components.h"
#include "../map/Map.h"
#include <iostream>

namespace openfranko::src::game {

map::Map *map;

SDL_Renderer *Game::renderer = nullptr;

entityComponentSystem::Manager manager;
auto &player(manager.addEntity());

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

  player.addComponent<
      entityComponentSystem::positionComponent::PositionComponent>(0, 0);
  player.addComponent<entityComponentSystem::spriteComponent::SpriteComponent>(
      "assets/00FF/00FF_006.bmp");
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
  manager.refresh();
  manager.update();

  if (player
          .getComponent<
              entityComponentSystem::positionComponent::PositionComponent>()
          .x() > 100) {
    player
        .getComponent<entityComponentSystem::spriteComponent::SpriteComponent>()
        .setTex("assets/0038/0038_009.bmp");
  };
}

void Game::render() {
  SDL_RenderClear(renderer);
  map->drawMap();
  manager.draw();
  SDL_RenderPresent(renderer);
}

void Game::clean() {
  SDL_DestroyWindow(m_window);
  SDL_DestroyRenderer(renderer);
  SDL_Quit();
  std::cout << "Game cleaned!" << std::endl;
}

} // namespace openfranko::src::game