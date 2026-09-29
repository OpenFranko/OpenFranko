#include "graphics/VideoSystem.h"

#include "graphics/IndexedRasterizer.h"

#include <algorithm>
#include <allegro.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <pc.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace openfranko::src::systems::graphics {
namespace {

constexpr int SCREEN_WIDTH = 376;
constexpr int SCREEN_HEIGHT = 282;
constexpr int PLANES = 4;
constexpr int SEQUENCER_PORT = 0x3C4;
constexpr int MAP_MASK = 0x02;
constexpr int PLANE_SHIFT = 8;
constexpr int DAC_SHIFT = 2;

volatile int vbls = 0;

void countVbl() { ++vbls; }
END_OF_FUNCTION(countVbl)

[[noreturn]] void throwError(const std::string &cause) {
  throw std::runtime_error("Video system error: " + cause + ": " +
                           allegro_error);
}

struct Placement {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  int step = 1;
  int sourceWidth = 0;
  int sourceHeight = 0;
};

bool operator!=(const Placement &left, const Placement &right) {
  return left.x != right.x || left.y != right.y || left.width != right.width ||
         left.height != right.height || left.step != right.step ||
         left.sourceWidth != right.sourceWidth ||
         left.sourceHeight != right.sourceHeight;
}

Placement place(const Display &display) {
  Placement placement;
  if (display.width <= 0 || display.height <= 0) {
    return placement;
  }
  placement.step = (display.width + SCREEN_WIDTH - 1) / SCREEN_WIDTH;
  placement.width = display.width / placement.step;
  placement.height = std::clamp(
      std::max(display.displayHeight, 1) / placement.step, 1, SCREEN_HEIGHT);
  placement.x = (SCREEN_WIDTH - placement.width) / 2 / PLANES * PLANES;
  placement.y = (SCREEN_HEIGHT - placement.height) / 2;
  placement.sourceWidth = display.width;
  placement.sourceHeight = display.height;
  return placement;
}

int sourceRow(const Placement &placement, int row) {
  return row * placement.sourceHeight / placement.height;
}

int brightness(uint16_t color) {
  return (color >> 8 & 0xF) + (color >> 4 & 0xF) + (color & 0xF);
}

int darkestSlot(const std::array<uint16_t, FRAME_COLORS> &palette) {
  const auto darkest = std::min_element(
      palette.begin(), palette.end(), [](uint16_t left, uint16_t right) {
        return brightness(left) < brightness(right);
      });
  return static_cast<int>(darkest - palette.begin());
}

int dacLevel(uint16_t color, int shift) {
  const int level = color >> shift & 0xF;
  return level << DAC_SHIFT | level >> DAC_SHIFT;
}

bool equalWords(const uint8_t *left, const uint8_t *right, std::size_t count) {
  std::size_t words = count / sizeof(uint32_t);
  bool same = true;
  if (words > 0) {
    asm volatile("cld\n\trepe cmpsl"
                 : "+S"(left), "+D"(right), "+c"(words), "=@ccz"(same)
                 :
                 : "memory");
  }
  for (std::size_t at = 0; same && at < count % sizeof(uint32_t); ++at) {
    same = left[at] == right[at];
  }
  return same;
}

uint32_t gatherWord(const uint8_t *source, int stride) {
  return static_cast<uint32_t>(source[0]) |
         static_cast<uint32_t>(source[stride]) << 8 |
         static_cast<uint32_t>(source[2 * stride]) << 16 |
         static_cast<uint32_t>(source[3 * stride]) << 24;
}

template <int Stride>
void writePlaneWords(uintptr_t &address, const uint8_t *&source, int words) {
  int pairs = words / 2;
  if (pairs > 0) {
    asm volatile(
        "1:\n\t"
        "movb %c[s3](%%esi), %%ah\n\t"
        "movb %c[s2](%%esi), %%al\n\t"
        "shll $16, %%eax\n\t"
        "movb %c[s1](%%esi), %%ah\n\t"
        "movb (%%esi), %%al\n\t"
        "movl %%eax, %%fs:(%%edi)\n\t"
        "movb %c[s7](%%esi), %%ah\n\t"
        "movb %c[s6](%%esi), %%al\n\t"
        "shll $16, %%eax\n\t"
        "movb %c[s5](%%esi), %%ah\n\t"
        "movb %c[s4](%%esi), %%al\n\t"
        "movl %%eax, %%fs:4(%%edi)\n\t"
        "addl %[pair], %%esi\n\t"
        "addl $8, %%edi\n\t"
        "decl %%ecx\n\t"
        "jnz 1b"
        : "+S"(source), "+D"(address), "+c"(pairs)
        : [s1] "i"(Stride), [s2] "i"(2 * Stride), [s3] "i"(3 * Stride),
          [s4] "i"(4 * Stride), [s5] "i"(5 * Stride), [s6] "i"(6 * Stride),
          [s7] "i"(7 * Stride), [pair] "i"(8 * Stride)
        : "eax", "cc", "memory");
  }
  if (words % 2 != 0) {
    bmp_write32(address, gatherWord(source, Stride));
    address += PLANES;
    source += PLANES * Stride;
  }
}

void writePlaneRow(uintptr_t address, const uint8_t *source, int count,
                   int step) {
  const int stride = PLANES * step;
  while (count > 0 && address % sizeof(uint32_t) != 0) {
    bmp_write8(address, *source);
    ++address;
    source += stride;
    --count;
  }
  const int words = count / PLANES;
  count -= words * PLANES;
  if (step == 1) {
    writePlaneWords<PLANES>(address, source, words);
  } else if (step == 2) {
    writePlaneWords<2 * PLANES>(address, source, words);
  } else {
    for (int word = 0; word < words; ++word) {
      bmp_write32(address, gatherWord(source, stride));
      address += PLANES;
      source += PLANES * stride;
    }
  }
  for (; count > 0; --count) {
    bmp_write8(address, *source);
    ++address;
    source += stride;
  }
}

} // namespace

struct VideoSystem::Window {
  IndexedRasterizer rasterizer{equalWords};
  IndexedFrame frame;
  Placement placement;
  bool fresh = true;
  int letterbox = -1;
  std::array<uint16_t, FRAME_COLORS> palette{};
  PALETTE dac{};
  std::vector<int> rows;
  int hertz = 0;
  int nextVbl = 0;
};

VideoSystem::VideoSystem() : m_window(std::make_unique<Window>()) {
  set_color_depth(8);
  if (set_gfx_mode(GFX_MODEX, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0) != 0) {
    throwError("Failed to open the 376x282 VGA screen");
  }
  LOCK_VARIABLE(vbls);
  LOCK_FUNCTION(countVbl);
  set_palette(black_palette);
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
  IndexedFrame &frame = window.frame;
  window.rasterizer.rasterize(m_shown, frame);
  const Placement placement = place(m_shown);
  const bool fresh = window.fresh || placement != window.placement;
  window.placement = placement;
  window.fresh = false;

  if (frame.palette != window.palette) {
    window.palette = frame.palette;
    for (std::size_t slot = 0; slot < FRAME_COLORS; ++slot) {
      window.dac[slot].r =
          static_cast<unsigned char>(dacLevel(frame.palette[slot], 8));
      window.dac[slot].g =
          static_cast<unsigned char>(dacLevel(frame.palette[slot], 4));
      window.dac[slot].b =
          static_cast<unsigned char>(dacLevel(frame.palette[slot], 0));
    }
    set_palette_range(window.dac, 0, static_cast<int>(FRAME_COLORS) - 1, FALSE);
  }
  const int letterbox = darkestSlot(frame.palette);
  const bool cleared = fresh || letterbox != window.letterbox;
  if (cleared) {
    clear_to_color(screen, letterbox);
    window.letterbox = letterbox;
  }

  window.rows.clear();
  for (int row = 0; row < placement.height; ++row) {
    const std::size_t shown =
        static_cast<std::size_t>(sourceRow(placement, row));
    if (cleared || frame.changedRows[shown]) {
      window.rows.push_back(row);
    }
  }
  if (window.rows.empty()) {
    return;
  }

  const std::size_t width = static_cast<std::size_t>(frame.width);
  bmp_select(screen);
  for (int plane = 0; plane < PLANES; ++plane) {
    const int count = (placement.width - plane + PLANES - 1) / PLANES;
    if (count <= 0) {
      continue;
    }
    outportw(SEQUENCER_PORT, (1 << (PLANE_SHIFT + plane)) | MAP_MASK);
    for (const int row : window.rows) {
      const uint8_t *source =
          frame.pixels.data() +
          static_cast<std::size_t>(sourceRow(placement, row)) * width +
          plane * placement.step;
      writePlaneRow(
          reinterpret_cast<uintptr_t>(screen->line[placement.y + row]) +
              placement.x / PLANES,
          source, count, placement.step);
    }
  }
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
