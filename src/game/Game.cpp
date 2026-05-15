#include "Game.h"
#include "../textureManager/TextureManager.h"
#include <iostream>

namespace openfranko::src::game {

SDL_Texture *titleTex;
SDL_Rect srcR, destR;

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
      m_renderer = SDL_CreateRenderer(m_window, -1, 0);
      if (m_renderer) {
        SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
        std::cout << "Renderer created!" << std::endl;
      }
      m_running = true;
    }
  } else {
    m_running = false;
  }

  titleTex = textureManager::TextureManager::LoadTexture("assets/03BA.bmp",
                                                         m_renderer);
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
  m_count++;
  destR.h = 512;
  destR.w = 640;
  destR.x = m_count;
  std::cout << m_count << std::endl;
}

void Game::render() {
  SDL_RenderClear(m_renderer);
  SDL_RenderCopy(m_renderer, titleTex, NULL, &destR);
  SDL_RenderPresent(m_renderer);
}

void Game::clean() {
  SDL_DestroyWindow(m_window);
  SDL_DestroyRenderer(m_renderer);
  SDL_Quit();
  std::cout << "Game cleaned!" << std::endl;
}

} // namespace openfranko::src::game