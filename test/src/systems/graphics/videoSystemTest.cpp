#include "../../../../src/systems/graphics/VideoSystem.h"

#include "../HeadlessSdl.h"

#include <SDL2/SDL.h>
#include <catch2/catch_all.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

using namespace openfranko::src::systems::graphics;
using namespace openfranko::test::src::systems;

namespace {

using Clock = std::chrono::steady_clock;

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr int MIDDLE_X = WINDOW_WIDTH / 2;
constexpr int MIDDLE_Y = WINDOW_HEIGHT / 2;

constexpr uint16_t RED = 0xF00;
constexpr uint16_t GREEN = 0x0F0;
constexpr uint16_t BLUE = 0x00F;
constexpr uint16_t WHITE = 0xFFF;
constexpr uint32_t BLACK_PIXEL = 0xFF000000;

Display frame(int width, int height, int displayHeight, uint16_t color) {
  Display display;
  display.width = width;
  display.height = height;
  display.displayHeight = displayHeight;
  display.border = color;
  return display;
}

Display splitFrame(int width, int height, uint16_t left, uint16_t right) {
  Display display = frame(width, height, height, left);
  Layer layer = solidLayer(right, 0, height, width / 2);
  layer.left = width / 2;
  display.layers.push_back(layer);
  return display;
}

void pressAltReturn() {
  pushKey(SDL_KEYDOWN, SDL_SCANCODE_RETURN, SDLK_RETURN, KMOD_LALT);
}

struct Screen {
  HeadlessSdl sdl;
  VideoSystem video;

  void present(const Display &display) {
    video.show(display);
    video.sync();
  }
};

} // namespace

#if defined(__DJGPP__) || defined(DJGPP)
SCENARIO("The DOS video system defaults to NTSC timing") {
  GIVEN("A DOS video system") {
    Screen screen;

    THEN("It starts at 60 Hz so FreeDOS matches the real hardware") {
      REQUIRE(screen.video.isNtsc());
      REQUIRE(screen.video.refreshRate() == NTSC_HERTZ);
    }
  }
}
#endif

SCENARIO("The video system opens an 800x600 window without a pointer") {
  GIVEN("A video system") {
    Screen screen;
    SDL_Window *window = openWindow();

    THEN("Its window is a resizable OpenFranko window with a renderer") {
      REQUIRE(window != nullptr);
      REQUIRE(std::string(SDL_GetWindowTitle(window)) == "OpenFranko");
      int width = 0;
      int height = 0;
      SDL_GetWindowSize(window, &width, &height);
      REQUIRE(width == WINDOW_WIDTH);
      REQUIRE(height == WINDOW_HEIGHT);
      REQUIRE((SDL_GetWindowFlags(window) & SDL_WINDOW_RESIZABLE) != 0);
      REQUIRE_FALSE(isFullscreen());
      REQUIRE(SDL_GetRenderer(window) != nullptr);
      REQUIRE(SDL_ShowCursor(SDL_QUERY) == SDL_DISABLE);
    }

    THEN("It starts on PAL, draws sprites itself and redraws whole frames") {
      REQUIRE_FALSE(screen.video.isNtsc());
      REQUIRE(screen.video.refreshRate() == PAL_HERTZ);
      REQUIRE(screen.video.showsSprites());
      REQUIRE_FALSE(screen.video.diffsFrames());
      REQUIRE_FALSE(screen.video.readsBuffersLive());
    }

    WHEN("NTSC is switched on") {
      screen.video.setNtsc(true);

      THEN("It refreshes at 60 Hz") {
        REQUIRE(screen.video.isNtsc());
        REQUIRE(screen.video.refreshRate() == NTSC_HERTZ);
      }
    }
  }
}

SCENARIO("A shown frame fills the window at its display aspect") {
  GIVEN("A video system") {
    Screen screen;

    WHEN("A frame wider than the window is synced") {
      screen.present(frame(320, 200, 200, RED));

      THEN("It spans the width between black bands above and below") {
        REQUIRE(windowPixel(MIDDLE_X, MIDDLE_Y) == toArgb(RED));
        REQUIRE(windowPixel(0, MIDDLE_Y) == toArgb(RED));
        REQUIRE(windowPixel(WINDOW_WIDTH - 1, MIDDLE_Y) == toArgb(RED));
        REQUIRE(windowPixel(MIDDLE_X, 25) == BLACK_PIXEL);
        REQUIRE(windowPixel(MIDDLE_X, 575) == BLACK_PIXEL);
      }
    }

    WHEN("A frame with a tall display height is synced") {
      screen.present(frame(320, 200, 400, RED));

      THEN("It spans the height between black bands left and right") {
        REQUIRE(windowPixel(MIDDLE_X, MIDDLE_Y) == toArgb(RED));
        REQUIRE(windowPixel(MIDDLE_X, 0) == toArgb(RED));
        REQUIRE(windowPixel(MIDDLE_X, WINDOW_HEIGHT - 1) == toArgb(RED));
        REQUIRE(windowPixel(80, MIDDLE_Y) == BLACK_PIXEL);
        REQUIRE(windowPixel(720, MIDDLE_Y) == BLACK_PIXEL);
      }
    }

    WHEN("A frame is shown and changed before the sync") {
      Display display = frame(320, 200, 200, RED);
      screen.video.show(display);
      display.border = BLUE;
      screen.video.sync();

      THEN("The frame as it was shown is drawn") {
        REQUIRE(windowPixel(MIDDLE_X, MIDDLE_Y) == toArgb(RED));
      }
    }

    WHEN("A layer carries a sprite") {
      const std::vector<uint8_t> background(8 * 4, 0);
      const std::vector<uint8_t> sprite(4 * 4, 1);
      Display display = frame(8, 4, 4, 0);
      Layer layer;
      layer.pixels = background.data();
      layer.stride = 8;
      layer.sourceColumns = 8;
      layer.sourceRows = 4;
      layer.columns = 8;
      layer.rows = 4;
      layer.palette = {GREEN, RED};
      layer.carriesSprites = true;
      layer.sprites.push_back({sprite.data(), 4, 4, 4, 0});
      display.layers.push_back(layer);
      screen.present(display);

      THEN("The window shows the sprite over the layer") {
        REQUIRE(windowPixel(200, MIDDLE_Y) == toArgb(GREEN));
        REQUIRE(windowPixel(600, MIDDLE_Y) == toArgb(RED));
      }
    }
  }
}

SCENARIO("Frames keep being presented until they change") {
  GIVEN("A video system showing a green frame") {
    Screen screen;
    screen.present(frame(8, 4, 4, GREEN));

    THEN("Another sync without a new frame shows it again") {
      screen.video.sync();
      REQUIRE(windowPixel(MIDDLE_X, MIDDLE_Y) == toArgb(GREEN));
    }

    WHEN("A frame twice the size, blue and red, is shown") {
      screen.present(splitFrame(16, 8, BLUE, RED));

      THEN("All of it is drawn") {
        REQUIRE(windowPixel(200, MIDDLE_Y) == toArgb(BLUE));
        REQUIRE(windowPixel(600, MIDDLE_Y) == toArgb(RED));
      }

      AND_WHEN("A frame smaller than the first is shown") {
        screen.present(frame(2, 1, 1, WHITE));

        THEN("It fills the same band") {
          REQUIRE(windowPixel(200, MIDDLE_Y) == toArgb(WHITE));
          REQUIRE(windowPixel(600, MIDDLE_Y) == toArgb(WHITE));
        }
      }
    }

    WHEN("The screen is cleared") {
      screen.video.clear();
      screen.video.sync();

      THEN("The window is black") {
        REQUIRE(windowPixel(MIDDLE_X, MIDDLE_Y) == BLACK_PIXEL);
      }
    }

    WHEN("An empty frame is shown") {
      screen.present(Display{});

      THEN("The window is black") {
        REQUIRE(windowPixel(MIDDLE_X, MIDDLE_Y) == BLACK_PIXEL);
      }
    }
  }
}

SCENARIO("Alt+Return switches fullscreen at the next sync") {
  GIVEN("A video system showing a red frame") {
    Screen screen;
    const Display red = frame(320, 200, 200, RED);
    screen.present(red);
    SDL_DisplayMode desktop{};
    SDL_GetDesktopDisplayMode(0, &desktop);

    WHEN("Alt+Return is pressed") {
      pressAltReturn();

      THEN("The window waits for the sync") { REQUIRE_FALSE(isFullscreen()); }

      AND_WHEN("The next frame is synced") {
        screen.video.sync();

        THEN("The window covers the desktop and the frame is fitted to it") {
          REQUIRE(isFullscreen());
          int width = 0;
          int height = 0;
          SDL_GetWindowSize(openWindow(), &width, &height);
          REQUIRE(width == desktop.w);
          REQUIRE(height == desktop.h);
          REQUIRE(windowPixel(desktop.w / 2, desktop.h / 2) == toArgb(RED));
          REQUIRE(windowPixel(desktop.w - 1, desktop.h / 2) == toArgb(RED));
          REQUIRE(windowPixel(desktop.w / 2, 0) == BLACK_PIXEL);
        }

        AND_WHEN("NTSC is switched on") {
          screen.video.setNtsc(true);
          screen.video.sync();

          THEN("The window stays fullscreen") { REQUIRE(isFullscreen()); }
        }

        AND_WHEN("Alt+Return is pressed again") {
          pressAltReturn();
          screen.video.sync();

          THEN("The window is back to 800x600") {
            REQUIRE_FALSE(isFullscreen());
            int width = 0;
            int height = 0;
            SDL_GetWindowSize(openWindow(), &width, &height);
            REQUIRE(width == WINDOW_WIDTH);
            REQUIRE(height == WINDOW_HEIGHT);
          }
        }
      }
    }

    WHEN("Return, a held Alt+Return, a released Alt+Return and Alt+A arrive") {
      pushKey(SDL_KEYDOWN, SDL_SCANCODE_RETURN, SDLK_RETURN);
      pushKey(SDL_KEYDOWN, SDL_SCANCODE_RETURN, SDLK_RETURN, KMOD_LALT, true);
      pushKey(SDL_KEYUP, SDL_SCANCODE_RETURN, SDLK_RETURN, KMOD_LALT);
      pushKey(SDL_KEYDOWN, SDL_SCANCODE_A, SDLK_a, KMOD_LALT);
      screen.video.sync();

      THEN("The window stays as it is") { REQUIRE_FALSE(isFullscreen()); }
    }
  }
}

SCENARIO("sync paces the frames at the display rate") {
  GIVEN("A PAL video system") {
    Screen screen;

    WHEN("Six frames are synced") {
      const Clock::time_point start = Clock::now();
      for (int frame = 0; frame < 6; ++frame) {
        screen.video.sync();
      }
      const Clock::duration elapsed = Clock::now() - start;

      THEN("They last at least three 50 Hz frames and well under a second") {
        REQUIRE(elapsed >= std::chrono::milliseconds(60));
        REQUIRE(elapsed < std::chrono::seconds(1));
      }
    }
  }

  GIVEN("An NTSC video system") {
    Screen screen;
    screen.video.setNtsc(true);

    WHEN("Six frames are synced") {
      const Clock::time_point start = Clock::now();
      for (int frame = 0; frame < 6; ++frame) {
        screen.video.sync();
      }
      const Clock::duration elapsed = Clock::now() - start;

      THEN("They last at least three 60 Hz frames and well under a second") {
        REQUIRE(elapsed >= std::chrono::milliseconds(50));
        REQUIRE(elapsed < std::chrono::seconds(1));
      }
    }
  }
}

SCENARIO("A video system that cannot open reports why") {
  GIVEN("SDL told to use a video driver that does not exist") {
    HeadlessSdl sdl;
    SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER, "openFrankoNoDriver",
                            SDL_HINT_OVERRIDE);

    THEN("Construction fails on SDL") {
      REQUIRE_THROWS_WITH(VideoSystem{},
                          "Video system error: SDL initialization failed");
    }
  }

  GIVEN("SDL told to use a renderer that does not exist") {
    HeadlessSdl sdl;
    SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "openFrankoNoRenderer",
                            SDL_HINT_OVERRIDE);

    THEN("Construction fails on the renderer and leaves no window behind") {
      REQUIRE_THROWS_WITH(VideoSystem{},
                          "Video system error: Renderer creation failed");
      REQUIRE(openWindow() == nullptr);
    }
  }
}
