#ifndef MAP_MAP_H_
#define MAP_MAP_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>

namespace openfranko {
namespace src {
namespace map {

class Map {
public:
  Map();
  ~Map();

  static void loadMap(const std::string &path, int sizeX, int sizeY);

private:
};

} // namespace map
} // namespace src
} // namespace openfranko

#endif // MAP_MAP_H_