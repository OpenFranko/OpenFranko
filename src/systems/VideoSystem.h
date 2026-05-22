#ifndef SYSTEMS_VIDEOSYSTEM_H_
#define SYSTEMS_VIDEOSYSTEM_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace openfranko {
namespace src {
namespace systems {

class VideoSystem {
public:
  VideoSystem();
  ~VideoSystem();

  void createScreen(int screenId, int width, int height);
  void switchScreen(int screenId);
  void loadAnimation(const std::string &name,
                     const std::vector<std::string> &framePaths);

  void clearAnimation(const std::string &name);

  void loadBackground(const std::string &path);

  void clearBackground();

  void drawAnimationFrame(const std::string &name, int x, int y, int frame,
                          SDL_RendererFlip flip = SDL_FLIP_NONE);

  void drawBackground();

  void sync();

private:
  struct VirtualScreen {
    int id;
    int width, height;
    SDL_Texture *targetTexture;
  };

  struct AnimationFrame {
    SDL_Texture *texture;
    int width;
    int height;
    int hotspotX;
    int hotspotY;
  };

  SDL_Texture *loadTexture(const char *fileName);

  AnimationFrame loadFrame(const std::string &path);

  std::map<std::string, std::vector<AnimationFrame>> animationStates;
  SDL_Texture *background;
  std::unordered_map<int, VirtualScreen> screens;

  SDL_Window *window;
  SDL_Renderer *renderer;

  int currentScreenId;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_VIDEOSYSTEM_H_