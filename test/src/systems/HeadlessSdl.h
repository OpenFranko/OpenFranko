#ifndef TEST_SRC_SYSTEMS_HEADLESSSDL_H_
#define TEST_SRC_SYSTEMS_HEADLESSSDL_H_

#include <SDL2/SDL.h>

#include <cstdint>

namespace openfranko {
namespace test {
namespace src {
namespace systems {

class HeadlessSdl {
public:
  HeadlessSdl() {
    SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER, "dummy", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_AUDIODRIVER, "dummy", SDL_HINT_OVERRIDE);
    SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software",
                            SDL_HINT_OVERRIDE);
  }

  ~HeadlessSdl() { SDL_Quit(); }

  HeadlessSdl(const HeadlessSdl &) = delete;
  HeadlessSdl &operator=(const HeadlessSdl &) = delete;
};

inline SDL_Window *openWindow() {
  constexpr Uint32 LAST_WINDOW_ID = 100000;
  for (Uint32 id = 1; id <= LAST_WINDOW_ID; ++id) {
    if (SDL_Window *window = SDL_GetWindowFromID(id)) {
      return window;
    }
  }
  return nullptr;
}

inline uint32_t windowPixel(int x, int y) {
  uint32_t pixel = 0;
  const SDL_Rect area{x, y, 1, 1};
  SDL_RenderReadPixels(SDL_GetRenderer(openWindow()), &area,
                       SDL_PIXELFORMAT_ARGB8888, &pixel, sizeof(pixel));
  return pixel;
}

inline bool isFullscreen() {
  return (SDL_GetWindowFlags(openWindow()) & SDL_WINDOW_FULLSCREEN_DESKTOP) ==
         SDL_WINDOW_FULLSCREEN_DESKTOP;
}

inline void pushKey(Uint32 type, SDL_Scancode scancode, SDL_Keycode symbol,
                    Uint16 modifiers = KMOD_NONE, bool repeat = false) {
  SDL_Event event{};
  event.type = type;
  event.key.state = type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
  event.key.repeat = repeat ? 1 : 0;
  event.key.keysym.scancode = scancode;
  event.key.keysym.sym = symbol;
  event.key.keysym.mod = modifiers;
  SDL_PushEvent(&event);
}

inline void pushEvent(Uint32 type) {
  SDL_Event event{};
  event.type = type;
  SDL_PushEvent(&event);
}

} // namespace systems
} // namespace src
} // namespace test
} // namespace openfranko

#endif // TEST_SRC_SYSTEMS_HEADLESSSDL_H_
