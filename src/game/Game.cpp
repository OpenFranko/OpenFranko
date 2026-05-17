#include "Game.h"
#include "../entityComponentSystem/components/Components.h"
#include "../map/Map.h"
#include "../vector2d/Vector2D.h"
#include <iostream>

namespace openfranko::src::game {

map::Map *map;
entityComponentSystem::Manager manager;

SDL_Renderer *Game::renderer = nullptr;
SDL_Event Game::event;

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
      entityComponentSystem::transformComponent::TransformComponent>();
  player.addComponent<entityComponentSystem::spriteComponent::SpriteComponent>(
      "assets/00FF/00FF_006.bmp");
  player.addComponent<
      entityComponentSystem::keyboardController::KeyboardController>();
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