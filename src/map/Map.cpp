#include "Map.h"
#include "../game/Game.h"
#include <fstream>

namespace openfranko::src::map {

Map::Map() {}

Map::~Map() {}

void Map::loadMap(const std::string &path, int sizeX, int sizeY) {
  char tile;
  std::fstream mapFile;
  mapFile.open(path);

  for (int y = 0; y < sizeY; y++) {
    for (int x = 0; x < sizeX; x++) {
      mapFile.get(tile);
      game::Game::addTile(atoi(&tile), x * 32, y * 32);
      mapFile.ignore();
    }
  }

  mapFile.close();
}

} // namespace openfranko::src::map