#include "VideoSystem.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace openfranko::src::systems {
namespace {

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Video system error: " + cause);
}

constexpr auto WINDOW_NAME = "OpenFranko";
constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;
constexpr int PAL_HERTZ = 50;
constexpr int NTSC_HERTZ = 60;

} // namespace

struct VideoSystem::Window {
  ~Window() {
    if (texture) {
      SDL_DestroyTexture(texture);
    }
    if (renderer) {
      SDL_DestroyRenderer(renderer);
    }
    if (window) {
      SDL_DestroyWindow(window);
    }
  }

  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Texture *texture = nullptr;
  int textureWidth = 0;
  int textureHeight = 0;
};

VideoSystem::VideoSystem() : window(std::make_unique<Window>()) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
    throwError("SDL initialization failed");
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

  window->window = SDL_CreateWindow(
      WINDOW_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
      WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!window->window) {
    throwError("Window creation failed");
  }
  SDL_ShowCursor(SDL_DISABLE);

  window->renderer = SDL_CreateRenderer(
      window->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!window->renderer) {
    throwError("Renderer creation failed");
  }
}

VideoSystem::~VideoSystem() = default;

void VideoSystem::show(const Display &display) {
  shown = display;
  frameChanged = true;
}

void VideoSystem::clear() {
  shown = Display{};
  frameChanged = true;
}

void VideoSystem::sync() {
  SDL_Renderer *renderer = window->renderer;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  if (shown.width <= 0 || shown.height <= 0) {
    SDL_RenderPresent(renderer);
    return;
  }

  if (frameChanged) {
    rasterize(shown, frame);
    if (window->textureWidth != shown.width ||
        window->textureHeight != shown.height) {
      if (window->texture) {
        SDL_DestroyTexture(window->texture);
      }
      window->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STREAMING,
                                          shown.width, shown.height);
      if (!window->texture) {
        throwError("Failed to create frame texture: " +
                   std::string(SDL_GetError()));
      }
      SDL_SetTextureBlendMode(window->texture, SDL_BLENDMODE_NONE);
      window->textureWidth = shown.width;
      window->textureHeight = shown.height;
    }
    SDL_UpdateTexture(window->texture, nullptr, frame.data(),
                      shown.width * static_cast<int>(sizeof(uint32_t)));
    frameChanged = false;
  }

  int windowWidth, windowHeight;
  SDL_GetWindowSize(window->window, &windowWidth, &windowHeight);

  const float frameAspect =
      static_cast<float>(shown.width) / std::max(shown.displayHeight, 1);
  const float windowAspect = static_cast<float>(windowWidth) / windowHeight;

  SDL_Rect dstRect;
  if (windowAspect > frameAspect) {
    dstRect.h = windowHeight;
    dstRect.w = static_cast<int>(windowHeight * frameAspect);
    dstRect.x = (windowWidth - dstRect.w) / 2;
    dstRect.y = 0;
  } else {
    dstRect.w = windowWidth;
    dstRect.h = static_cast<int>(windowWidth / frameAspect);
    dstRect.x = 0;
    dstRect.y = (windowHeight - dstRect.h) / 2;
  }

  SDL_RenderCopy(renderer, window->texture, nullptr, &dstRect);
  SDL_RenderPresent(renderer);
}

void VideoSystem::setNtsc(bool enabled) { ntsc = enabled; }

bool VideoSystem::isNtsc() const { return ntsc; }

int VideoSystem::refreshRate() const { return ntsc ? NTSC_HERTZ : PAL_HERTZ; }

} // namespace openfranko::src::systems
