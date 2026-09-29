#include "graphics/VideoSystem.h"

#include <algorithm>
#include <allegro.h>
#include <array>
#include <climits>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace openfranko::src::systems::graphics {
namespace {

constexpr int SCREEN_WIDTH = 640;
constexpr int SCREEN_HEIGHT = 480;
constexpr std::array<int, 4> COLOR_DEPTHS = {16, 15, 32, 24};
constexpr std::size_t AMIGA_COLORS = 4096;

struct Area {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

volatile int vbls = 0;

void countVbl() { ++vbls; }
END_OF_FUNCTION(countVbl)

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Video system error: " + cause + ": " +
                           allegro_error);
}

bool openScreen() {
  for (const int depth : COLOR_DEPTHS) {
    set_color_depth(depth);
    if (set_gfx_mode(GFX_AUTODETECT, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0) == 0) {
      return true;
    }
  }
  return false;
}

bool operator!=(const Area &left, const Area &right) {
  return left.x != right.x || left.y != right.y || left.width != right.width ||
         left.height != right.height;
}

Area fitArea(const Display &display) {
  if (display.width <= 0 || display.height <= 0) {
    return {};
  }
  const float frameAspect =
      static_cast<float>(display.width) / std::max(display.displayHeight, 1);
  const float screenAspect = static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT;
  Area area;
  if (screenAspect > frameAspect) {
    area.height = SCREEN_HEIGHT;
    area.width = static_cast<int>(SCREEN_HEIGHT * frameAspect);
    area.x = (SCREEN_WIDTH - area.width) / 2;
  } else {
    area.width = SCREEN_WIDTH;
    area.height = static_cast<int>(SCREEN_WIDTH / frameAspect);
    area.y = (SCREEN_HEIGHT - area.height) / 2;
  }
  return area;
}

std::size_t amigaColor(uint32_t argb) {
  return (argb >> 12 & 0xF00) | (argb >> 8 & 0xF0) | (argb >> 4 & 0xF);
}

int bytesPerPixel(BITMAP *bitmap) {
  return (bitmap_color_depth(bitmap) + CHAR_BIT - 1) / CHAR_BIT;
}

template <typename Pixel>
void convertRows(BITMAP *frame, const std::vector<uint32_t> &argb,
                 const std::vector<int> &colors) {
  for (int y = 0; y < frame->h; ++y) {
    Pixel *row = reinterpret_cast<Pixel *>(frame->line[y]);
    const uint32_t *source =
        argb.data() + static_cast<std::ptrdiff_t>(y) * frame->w;
    for (int x = 0; x < frame->w; ++x) {
      row[x] = static_cast<Pixel>(colors[amigaColor(source[x])]);
    }
  }
}

void convertFrame(BITMAP *frame, const std::vector<uint32_t> &argb,
                  const std::vector<int> &colors) {
  switch (bytesPerPixel(frame)) {
  case sizeof(uint16_t):
    convertRows<uint16_t>(frame, argb, colors);
    break;
  case sizeof(uint32_t):
    convertRows<uint32_t>(frame, argb, colors);
    break;
  default:
    for (int y = 0; y < frame->h; ++y) {
      for (int x = 0; x < frame->w; ++x) {
        putpixel(frame, x, y,
                 colors[amigaColor(
                     argb[static_cast<std::size_t>(y) * frame->w + x])]);
      }
    }
    break;
  }
}

} // namespace

struct VideoSystem::Window {
  ~Window() {
    if (frame) {
      destroy_bitmap(frame);
    }
  }

  BITMAP *frame = nullptr;
  std::vector<int> colors;
  Area area;
  int hertz = 0;
  int nextVbl = 0;
};

VideoSystem::VideoSystem() : m_window(std::make_unique<Window>()) {
  if (!openScreen()) {
    throwError("Failed to open a high colour screen");
  }
  LOCK_VARIABLE(vbls);
  LOCK_FUNCTION(countVbl);
  m_window->colors.reserve(AMIGA_COLORS);
  for (std::size_t color = 0; color < AMIGA_COLORS; ++color) {
    const uint32_t argb = toArgb(static_cast<uint16_t>(color));
    m_window->colors.push_back(
        makecol(argb >> 16 & 0xFF, argb >> 8 & 0xFF, argb & 0xFF));
  }
}

VideoSystem::~VideoSystem() {
  remove_int(countVbl);
  set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
}

void VideoSystem::present() {
  if (!m_frameChanged) {
    return;
  }
  m_frameChanged = false;
  Window &window = *m_window;
  const Area area = fitArea(m_shown);
  if (area != window.area) {
    clear_bitmap(screen);
    window.area = area;
  }
  if (m_shown.width <= 0 || m_shown.height <= 0) {
    return;
  }

  rasterize(m_shown, m_frame);
  if (!window.frame || window.frame->w != m_shown.width ||
      window.frame->h != m_shown.height) {
    if (window.frame) {
      destroy_bitmap(window.frame);
    }
    window.frame = create_bitmap(m_shown.width, m_shown.height);
    if (!window.frame) {
      throwError("Failed to create frame bitmap");
    }
  }
  convertFrame(window.frame, m_frame, window.colors);
  stretch_blit(window.frame, screen, 0, 0, m_shown.width, m_shown.height,
               area.x, area.y, area.width, area.height);
}

void VideoSystem::waitVbl() {
  Window &window = *m_window;
  const int hertz = refreshRate();
  if (hertz != window.hertz) {
    install_int_ex(countVbl, BPS_TO_TIMER(hertz));
    window.hertz = hertz;
    window.nextVbl = vbls;
  }
  ++window.nextVbl;
  while (vbls < window.nextVbl) {
  }
  if (vbls - window.nextVbl > 1) {
    window.nextVbl = vbls;
  }
}

} // namespace openfranko::src::systems::graphics
