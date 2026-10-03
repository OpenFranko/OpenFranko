#include "graphics/VideoSystem.h"

#include "graphics/ScreenMode.h"

#include <SDL2/SDL.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace openfranko::src::systems::graphics {
namespace {

using Clock = std::chrono::steady_clock;

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Video system error: " + cause);
}

constexpr auto WINDOW_NAME = "OpenFranko";
constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;
constexpr SDL_Keycode FULLSCREEN_KEY = SDLK_RETURN;
constexpr int FULLSCREEN_MODIFIERS = KMOD_ALT;
constexpr int VSYNC_FRAME_PERCENT = 75;
constexpr int PERCENT = 100;

ScreenMode screenMode(const SDL_DisplayMode &mode) {
  return {mode.w, mode.h, mode.refresh_rate};
}

} // namespace

struct VideoSystem::Window {
  ~Window() {
    SDL_DelEventWatch(watchKeys, this);
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

  static int SDLCALL watchKeys(void *window, SDL_Event *event);
  void toggleFullscreen(int hertz);
  void enterFullscreen(int hertz);
  ScreenMode displayMode() const;

  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Texture *texture = nullptr;
  int textureWidth = 0;
  int textureHeight = 0;
  Clock::time_point nextFrame = Clock::now();
  Clock::time_point frameEnd = Clock::now();
  std::atomic<bool> toggleWanted{false};
  bool fullscreen = false;
  int fullscreenHertz = 0;
};

int SDLCALL VideoSystem::Window::watchKeys(void *window, SDL_Event *event) {
  const SDL_KeyboardEvent &key = event->key;
  if (event->type == SDL_KEYDOWN && key.repeat == 0 &&
      key.keysym.sym == FULLSCREEN_KEY &&
      (key.keysym.mod & FULLSCREEN_MODIFIERS) != 0) {
    static_cast<Window *>(window)->toggleWanted = true;
  }
  return 1;
}

void VideoSystem::Window::toggleFullscreen(int hertz) {
  if (fullscreen) {
    SDL_SetWindowFullscreen(window, 0);
    fullscreen = false;
    return;
  }
  enterFullscreen(hertz);
}

void VideoSystem::Window::enterFullscreen(int hertz) {
  const int display = SDL_GetWindowDisplayIndex(window);
  SDL_DisplayMode desktop{};
  SDL_GetDesktopDisplayMode(display, &desktop);
  std::vector<SDL_DisplayMode> modes;
  std::vector<ScreenMode> screenModes;
  for (int index = 0; index < SDL_GetNumDisplayModes(display); ++index) {
    SDL_DisplayMode mode{};
    if (SDL_GetDisplayMode(display, index, &mode) == 0) {
      modes.push_back(mode);
      screenModes.push_back(screenMode(mode));
    }
  }
  const auto chosen = fullscreenMode(screenModes, screenMode(desktop), hertz);
  if (chosen) {
    SDL_SetWindowFullscreen(window, 0);
    SDL_SetWindowDisplayMode(window, &modes[*chosen]);
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
  } else {
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  }
  fullscreen = true;
  fullscreenHertz = hertz;
}

ScreenMode VideoSystem::Window::displayMode() const {
  SDL_DisplayMode mode{};
  SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(window), &mode);
  return screenMode(mode);
}

VideoSystem::VideoSystem() : m_window(std::make_unique<Window>()) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
    throwError("SDL initialization failed");
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

  m_window->window = SDL_CreateWindow(
      WINDOW_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
      WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (!m_window->window) {
    throwError("Window creation failed");
  }
  SDL_ShowCursor(SDL_DISABLE);

  m_window->renderer =
      SDL_CreateRenderer(m_window->window, -1,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!m_window->renderer) {
    throwError("Renderer creation failed");
  }
  SDL_AddEventWatch(Window::watchKeys, m_window.get());
}

VideoSystem::~VideoSystem() = default;

bool VideoSystem::showsSprites() const { return true; }

void VideoSystem::present() {
  if (m_window->toggleWanted.exchange(false)) {
    m_window->toggleFullscreen(refreshRate());
  } else if (m_window->fullscreen &&
             m_window->fullscreenHertz != refreshRate()) {
    m_window->enterFullscreen(refreshRate());
  }
  SDL_Renderer *renderer = m_window->renderer;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  if (m_shown.width <= 0 || m_shown.height <= 0) {
    SDL_RenderPresent(renderer);
    return;
  }

  if (m_frameChanged) {
    rasterize(m_shown, m_frame);
    if (m_window->textureWidth != m_shown.width ||
        m_window->textureHeight != m_shown.height) {
      if (m_window->texture) {
        SDL_DestroyTexture(m_window->texture);
      }
      m_window->texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            m_shown.width, m_shown.height);
      if (!m_window->texture) {
        throwError("Failed to create frame texture: " +
                   std::string(SDL_GetError()));
      }
      SDL_SetTextureBlendMode(m_window->texture, SDL_BLENDMODE_NONE);
      m_window->textureWidth = m_shown.width;
      m_window->textureHeight = m_shown.height;
    }
    SDL_UpdateTexture(m_window->texture, nullptr, m_frame.data(),
                      m_shown.width * static_cast<int>(sizeof(uint32_t)));
    m_frameChanged = false;
  }

  int windowWidth, windowHeight;
  SDL_GetWindowSize(m_window->window, &windowWidth, &windowHeight);

  const float frameAspect =
      static_cast<float>(m_shown.width) / std::max(m_shown.displayHeight, 1);
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

  SDL_RenderCopy(renderer, m_window->texture, nullptr, &dstRect);
  SDL_RenderPresent(renderer);
}

void VideoSystem::waitVbl() {
  const auto frameTime = std::chrono::duration_cast<Clock::duration>(
      std::chrono::duration<double>(1.0 / refreshRate()));
  m_window->nextFrame += frameTime;
  const Clock::time_point now = Clock::now();
  const bool vsynced =
      isRefreshedAt(m_window->displayMode(), refreshRate()) &&
      now - m_window->frameEnd >= frameTime * VSYNC_FRAME_PERCENT / PERCENT;
  if (vsynced) {
    m_window->nextFrame = now;
  } else if (m_window->nextFrame > now) {
    std::this_thread::sleep_until(m_window->nextFrame);
  } else if (now - m_window->nextFrame > frameTime) {
    m_window->nextFrame = now;
  }
  m_window->frameEnd = Clock::now();
}

} // namespace openfranko::src::systems::graphics
