#ifndef SYSTEMS_VIDEOSYSTEM_H_
#define SYSTEMS_VIDEOSYSTEM_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cstdint>
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

  VideoSystem(const VideoSystem &) = delete;
  VideoSystem &operator=(const VideoSystem &) = delete;

  void createScreen(int screenId, int width, int height);
  void switchScreen(int screenId);
  void destroyScreen(int screenId);
  void fillScreen(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
  void sync();

  void loadImage(const std::string &name, const std::string &path,
                 bool applyColorKey = false);
  void clearImage(const std::string &name);
  void drawImage(const std::string &name, int x, int y,
                 SDL_RendererFlip flip = SDL_FLIP_NONE);

  void loadIndexedImage(const std::string &name, const std::string &path);
  void loadMaskedImage(const std::string &name, const std::string &path);
  std::vector<uint16_t> getImagePalette(const std::string &name) const;
  void setImagePalette(const std::string &name,
                       const std::vector<uint16_t> &palette);
  void xorImageRect(const std::string &name, int x, int y, int width,
                    int height, uint8_t mask);
  void updateFrameImage(const std::string &name, int width, int height,
                        const std::vector<uint32_t> &argb);

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
    SDL_Surface *indexedSurface = nullptr;
  };

  const Image &findIndexedImage(const std::string &name) const;
  Image &findIndexedImage(const std::string &name);
  void refreshTexture(Image &image, const std::string &name);
  void addIndexedImage(const std::string &name, const std::string &path,
                       bool colorZeroTransparent);

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