#include "StageFrame.h"

namespace openfranko::src::engine::street {
namespace {

constexpr int PANEL_DISPLAY_Y = 270;
constexpr effects::AmigaColor BORDER = 0x555;

const effects::AmigaPalette LEVEL_PALETTE = {
    0x555, 0xAAA, 0x666, 0xFAA, 0x083, 0x902, 0xB95, 0x760,
    0x063, 0x000, 0x520, 0x17A, 0x09E, 0x4DF, 0x777, 0xDDD};
const effects::AmigaPalette GREY_PALETTE = {
    0x555, 0xCCC, 0x888, 0xEEE, 0x444, 0x111, 0xBBB, 0x777,
    0x333, 0x000, 0x222, 0x666, 0x999, 0xDDD, 0xAAA, 0xFFF};
const effects::AmigaPalette PANEL_PALETTE = {0x555, 0x000, 0xF10, 0x666,
                                             0x888, 0x999, 0xAAA, 0xDDD};

uint32_t toArgb(effects::AmigaColor color) {
  const uint32_t r = ((color >> 8) & 0xF) * 17;
  const uint32_t g = ((color >> 4) & 0xF) * 17;
  const uint32_t b = (color & 0xF) * 17;
  return 0xFF000000u | r << 16 | g << 8 | b;
}

} // namespace

const effects::AmigaPalette &levelPalette(bool mono) {
  return mono ? GREY_PALETTE : LEVEL_PALETTE;
}

const effects::AmigaPalette &panelPalette() { return PANEL_PALETTE; }

void composeFrame(std::vector<uint32_t> &frame, const IndexedSurface *display,
                  const effects::AmigaPalette &palette,
                  const amal::Object &screenDisplay, int offsetX,
                  const StatusPanel *panel,
                  const effects::AmigaPalette &panelColors) {
  frame.assign(static_cast<std::size_t>(FRAME_WIDTH * FRAME_HEIGHT),
               toArgb(BORDER));
  if (!panel) {
    return;
  }
  const IndexedSurface &panelSurface = panel->surface();
  for (int row = 0; row < FRAME_HEIGHT; ++row) {
    uint32_t *line = frame.data() + row * FRAME_WIDTH;
    const int beam = DISPLAY_TOP + row;
    const int panelRow = beam - PANEL_DISPLAY_Y;
    if (panelRow >= 0 && panelRow < StatusPanel::VISIBLE_HEIGHT) {
      for (int x = 0; x < FRAME_WIDTH; ++x) {
        line[x] = toArgb(panelColors[panelSurface.pixel(x, panelRow)]);
      }
      continue;
    }
    const int screenRow = beam - screenDisplay.y;
    if (!display || screenRow < 0 || screenRow >= display->height()) {
      continue;
    }
    for (int x = 0; x < FRAME_WIDTH; ++x) {
      const int column = x + offsetX;
      if (column < display->width()) {
        line[x] = toArgb(palette[display->pixel(column, screenRow)]);
      }
    }
  }
}

} // namespace openfranko::src::engine::street
