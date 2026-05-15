#ifndef MAP_MAP_H_
#define MAP_MAP_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace openfranko {
namespace src {
namespace map {

class Map {
public:
  Map();
  ~Map();

  void loadMap(int arr[20][25]);
  void drawMap();

private:
  SDL_Rect src, dest;
  SDL_Texture *dirt;
  SDL_Texture *grass;
  SDL_Texture *water;

  int map[20][25];
};

} // namespace map
} // namespace src
} // namespace openfranko

#endif // MAP_MAP_H_