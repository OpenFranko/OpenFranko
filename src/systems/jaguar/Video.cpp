#include "Video.h"

#include "Hardware.h"

namespace openfranko::src::systems::jaguar {
namespace {

constexpr int NTSC_WIDTH = 1409;
constexpr int NTSC_MIDDLE = 823;
constexpr int NTSC_HEIGHT = 241;
constexpr int NTSC_VERTICAL_MIDDLE = 266;
constexpr int PAL_WIDTH = 1381;
constexpr int PAL_MIDDLE = 843;
constexpr int PAL_HEIGHT = 287;
constexpr int PAL_VERTICAL_MIDDLE = 322;
constexpr int NTSC_HERTZ = 60;
constexpr int PAL_HERTZ = 50;
constexpr int DISPLAY_BEGIN_OFFSET = 4;
constexpr int LIST_SWAP_DELAY = 4;
constexpr uint16_t SECOND_HALF = 0x400;
constexpr uint16_t NO_DISPLAY_END = 0xFFFF;
constexpr uint16_t HALF_LINE_MASK = 0x7FF;
constexpr uint16_t VIDEO_MODE =
    VMODE_ENABLE | VMODE_RGB16 | VMODE_CSYNC | VMODE_BGEN |
    static_cast<uint16_t>((PIXEL_CLOCKS - 1) << VMODE_PWIDTH_SHIFT);

int lineWidth(bool ntsc) { return ntsc ? NTSC_WIDTH : PAL_WIDTH; }

int halfLine() { return word(VC) & HALF_LINE_MASK; }

} // namespace

Geometry detectGeometry() {
  Geometry geometry;
  geometry.ntsc = (word(CONFIG) & CONFIG_NTSC) != 0;
  geometry.hertz = geometry.ntsc ? NTSC_HERTZ : PAL_HERTZ;
  const int height = geometry.ntsc ? NTSC_HEIGHT : PAL_HEIGHT;
  const int middle = geometry.ntsc ? NTSC_VERTICAL_MIDDLE : PAL_VERTICAL_MIDDLE;
  geometry.columns = lineWidth(geometry.ntsc) / PIXEL_CLOCKS;
  geometry.rows = height;
  geometry.firstHalfLine = middle - height;
  geometry.lastHalfLine = middle + height;
  geometry.vblankHalfLine = geometry.lastHalfLine + LIST_SWAP_DELAY;
  return geometry;
}

void setupVideo(const Geometry &geometry) {
  const int width = lineWidth(geometry.ntsc);
  const int middle = geometry.ntsc ? NTSC_MIDDLE : PAL_MIDDLE;
  const uint16_t begin =
      static_cast<uint16_t>(middle - width / 2 + DISPLAY_BEGIN_OFFSET);
  word(HDB1) = begin;
  word(HDB2) = begin;
  word(HDE) = static_cast<uint16_t>((width / 2 - 1) | SECOND_HALF);
  word(VDB) = static_cast<uint16_t>(geometry.firstHalfLine);
  word(VDE) = NO_DISPLAY_END;
  longWord(BORD1) = 0;
  word(BG) = 0;
  word(VMODE) = VIDEO_MODE;
}

void setOlp(uint32_t address) { longWord(OLP) = address << 16 | address >> 16; }

bool isBlanking(const Geometry &geometry) {
  const int line = halfLine();
  return line >= geometry.lastHalfLine || line < geometry.firstHalfLine;
}

void waitBlanking(const Geometry &geometry) {
  while (!isBlanking(geometry)) {
  }
}

void waitTopBlanking(const Geometry &geometry) {
  while (halfLine() < geometry.firstHalfLine) {
  }
  while (halfLine() >= geometry.firstHalfLine) {
  }
}

void waitDisplay(const Geometry &geometry) {
  while (isBlanking(geometry)) {
  }
}

} // namespace openfranko::src::systems::jaguar
