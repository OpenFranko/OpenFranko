#ifndef SYSTEMS_VIDEOSYSTEM_H_
#define SYSTEMS_VIDEOSYSTEM_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <map>
#include <string>
#include <unordered_map>

namespace openfranko {
namespace src {
namespace systems {

class VideoSystem {
public:
  VideoSystem();
  ~VideoSystem();

  void createScreen(int screenId, int width, int height);
  void switchScreen(int screenId);
  void fillScreen(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
  void sync();

  void loadImage(const std::string &name, const std::string &path,
                 bool applyColorKey = false);
  void clearImage(const std::string &name);
  void drawImage(const std::string &name, int x, int y,
                 SDL_RendererFlip flip = SDL_FLIP_NONE);

private:
  struct VirtualScreen {
    int id;
    int width, height;
    SDL_Texture *targetTexture;
  };

  struct Image {
    SDL_Texture *texture = nullptr;
    int width = 0;
    int height = 0;
    int hotspotX = 0;
    int hotspotY = 0;
  };

  Image loadImageFile(const std::string &path, bool applyColorKey);

  std::map<std::string, Image> imageStates;
  std::unordered_map<int, VirtualScreen> screens;

  SDL_Window *window;
  SDL_Renderer *renderer;

  int currentScreenId;
};

} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_VIDEOSYSTEM_H_