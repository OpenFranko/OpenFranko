#include "game/Game.h"

using namespace openfranko::src::game;

Game *game = nullptr;

int main() {
  game = new Game();

  game->init("OpenFranko", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320,
             256, false);

  while (game->running()) {
    game->handleEvents();
    game->update();
    game->render();
  }

  game->clean();

  return 0;
}
