#ifndef ENGINE_ENGINE_H_
#define ENGINE_ENGINE_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {

class Engine {
public:
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

  static SDL_Event event;

  Engine()
      : window(nullptr), renderer(nullptr), currentScreenId(0), running(true) {}
  ~Engine() { shutdown(); }

  bool init(const std::string &title, int windowWidth, int windowHeight) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
      return false;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
      return false;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window)
      return false;

    renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
      return false;

    return true;
  }

  void screenOpen(int screenId, int width, int height) {
    SDL_Texture *target =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                          SDL_TEXTUREACCESS_TARGET, width, height);

    screens[screenId] = {screenId, width, height, target};
    if (screens.size() == 1)
      screen(screenId);
  }

  void screen(int screenId) {
    if (screens.find(screenId) != screens.end()) {
      currentScreenId = screenId;
      SDL_SetRenderTarget(renderer, screens[currentScreenId].targetTexture);
    }
  }

  void loadAnimation(const std::string &name,
                     const std::vector<std::string> &framePaths) {
    std::vector<AnimationFrame> frames;

    for (auto &path : framePaths) {
      const auto frame = loadFrame(path);
      frames.emplace_back(frame);
    }

    animationStates.emplace(name, frames);
  }

  void hotspot(const std::string &name, size_t frameIndex, int x, int y) {
    if (animationStates.find(name) != animationStates.end()) {
      auto &frame = animationStates.at(name).at(frameIndex);
      frame.hotspotX = x;
      frame.hotspotY = y;
    }
  }

  void bob(const std::string &name, int x, int y, int frame) {
    if (animationStates.find(name) == animationStates.end())
      return;

    const auto frames = animationStates.at(name);
    const auto animFrame = frames.at(frame);

    SDL_Rect srcRect = {0, 0, animFrame.width, animFrame.height};
    SDL_Rect dstRect = {x - animFrame.hotspotX, y - animFrame.hotspotY,
                        animFrame.width, animFrame.height};

    SDL_RenderCopy(renderer, animFrame.texture, &srcRect, &dstRect);
  }

  void sprite(const std::string &name, int x, int y, int frame,
              SDL_RendererFlip flip = SDL_FLIP_NONE) {
    if (animationStates.find(name) == animationStates.end())
      return;

    const auto frames = animationStates.at(name);
    const auto animFrame = frames.at(frame);

    SDL_Rect srcRect = {0, 0, animFrame.width, animFrame.height};

    int currentHotX = animFrame.hotspotX;
    int currentHotY = animFrame.hotspotY;

    if (flip & SDL_FLIP_HORIZONTAL) {
      currentHotX = animFrame.width - animFrame.hotspotX;
    }

    SDL_Rect dstRect = {x - currentHotX, y - currentHotY, animFrame.width,
                        animFrame.height};

    SDL_Point pivot = {currentHotX, currentHotY};

    SDL_RenderCopyEx(renderer, animFrame.texture, &srcRect, &dstRect, 0.0,
                     &pivot, flip);
  }

  void cls(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0) {
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderClear(renderer);
  }

  void sync() {
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    auto &activeScr = screens[currentScreenId];

    int windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);

    float targetAspect = static_cast<float>(activeScr.width) / activeScr.height;
    float windowAspect = static_cast<float>(windowWidth) / windowHeight;

    SDL_Rect dstRect;

    if (windowAspect > targetAspect) {
      dstRect.h = windowHeight;
      dstRect.w = static_cast<int>(windowHeight * targetAspect);
      dstRect.x = (windowWidth - dstRect.w) / 2;
      dstRect.y = 0;
    } else {
      dstRect.w = windowWidth;
      dstRect.h = static_cast<int>(windowWidth / targetAspect);
      dstRect.x = 0;
      dstRect.y = (windowHeight - dstRect.h) / 2;
    }

    SDL_RenderCopy(renderer, activeScr.targetTexture, nullptr, &dstRect);
    SDL_RenderPresent(renderer);
    SDL_SetRenderTarget(renderer, activeScr.targetTexture);
  }

  bool loop() {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        running = false;
    }
    return running;
  }

  void shutdown() {
    for (auto &state : animationStates) {
      for (auto &frame : state.second) {
        SDL_DestroyTexture(frame.texture);
      }
    }
    for (auto &pair : screens) {
      SDL_DestroyTexture(pair.second.targetTexture);
    }
    animationStates.clear();
    screens.clear();

    if (renderer)
      SDL_DestroyRenderer(renderer);
    if (window)
      SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
  }

private:
  SDL_Texture *LoadTexture(const char *fileName) {
    SDL_Surface *tempSurface = IMG_Load(fileName);
    Uint32 colorKey = SDL_MapRGB(tempSurface->format, 85, 85, 85);

    if (SDL_SetColorKey(tempSurface, SDL_TRUE, colorKey) < 0) {
      std::cerr << "Unable to set color key! SDL Error: " << SDL_GetError()
                << std::endl;
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_FreeSurface(tempSurface);
    return tex;
  }

  AnimationFrame loadFrame(const std::string &path) {
    SDL_Texture *tex = LoadTexture(path.c_str());
    if (!tex) {
      std::cerr << "Error: Could not load bitmap " << path << "\n";
      return {};
    }

    int width, height;
    SDL_QueryTexture(tex, nullptr, nullptr, &width, &height);

    return AnimationFrame{tex, width, height, width / 2, height / 2};
  }

  std::map<std::string, std::vector<AnimationFrame>> animationStates;

  SDL_Window *window;
  SDL_Renderer *renderer;
  int currentScreenId;
  bool running;

  std::unordered_map<int, VirtualScreen> screens;
};

inline SDL_Event Engine::event;

} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ENGINE_H_