#include "StageFrame.h"

#include "../effects/AmigaDisplay.h"

#include <algorithm>

namespace openfranko::src::engine::street {
namespace {

constexpr effects::AmigaColor BORDER = 0x555;
constexpr uint32_t BLANK = 0xFF000000u;
constexpr int NTSC_SHIFT = 40;
constexpr int LACED_PLAY_SHIFT = 60;
constexpr int LACED_PANEL_SHIFT = 51;

const effects::AmigaPalette LEVEL_PALETTE = {
    0x555, 0xAAA, 0x666, 0xFAA, 0x083, 0x902, 0xB95, 0x760,
    0x063, 0x000, 0x520, 0x17A, 0x09E, 0x4DF, 0x777, 0xDDD};
const effects::AmigaPalette GREY_PALETTE = {
    0x555, 0xCCC, 0x888, 0xEEE, 0x444, 0x111, 0xBBB, 0x777,
    0x333, 0x000, 0x222, 0x666, 0x999, 0xDDD, 0xAAA, 0xFFF};
const effects::AmigaPalette PANEL_PALETTE = {0x555, 0x000, 0xF10, 0x666,
                                             0x888, 0x999, 0xAAA, 0xDDD};

int sys(const StageLayout &layout) { return layout.ntsc ? -1 : 0; }

int wyb(const StageLayout &layout) { return layout.laced ? -1 : 0; }

} // namespace

StageLayout stageLayout(const effects::GameOptions &options) {
  return {options.ntsc, options.tallScreen};
}

int playDisplayY(const StageLayout &layout) {
  return DISPLAY_TOP + NTSC_SHIFT * sys(layout) -
         LACED_PLAY_SHIFT * wyb(layout);
}

int panelDisplayY(const StageLayout &layout) {
  return PANEL_DISPLAY_Y + NTSC_SHIFT * sys(layout) +
         LACED_PANEL_SHIFT * wyb(layout);
}

int frameTop(const StageLayout &layout) {
  return DISPLAY_TOP + NTSC_SHIFT * sys(layout);
}

int rowsPerLine(const StageLayout &layout) { return layout.laced ? 2 : 1; }

int frameRows(const StageLayout &layout) {
  return FRAME_HEIGHT * rowsPerLine(layout);
}

void switchStandard(effects::GameOptions &options, amal::Object &screenDisplay,
                    bool ntsc) {
  if (options.ntsc == ntsc) {
    return;
  }
  options.ntsc = ntsc;
  screenDisplay.x = DISPLAY_X;
  screenDisplay.y = static_cast<int16_t>(playDisplayY(stageLayout(options)));
}

uint32_t toArgb(effects::AmigaColor color) {
  const uint32_t r = ((color >> 8) & 0xF) * 17;
  const uint32_t g = ((color >> 4) & 0xF) * 17;
  const uint32_t b = (color & 0xF) * 17;
  return 0xFF000000u | r << 16 | g << 8 | b;
}

const effects::AmigaPalette &levelPalette(bool mono) {
  return mono ? GREY_PALETTE : LEVEL_PALETTE;
}

const effects::AmigaPalette &panelPalette() { return PANEL_PALETTE; }

void composeFrame(std::vector<uint32_t> &frame, const IndexedSurface *display,
                  const effects::AmigaPalette &palette,
                  const amal::Object &screenDisplay, int offsetX,
                  const StatusPanel *panel,
                  const effects::AmigaPalette &panelColors,
                  const StageLayout &layout) {
  const int rows = frameRows(layout);
  const int perLine = rowsPerLine(layout);
  const int top = frameTop(layout);
  frame.assign(static_cast<std::size_t>(FRAME_WIDTH * rows), toArgb(BORDER));
  if (!panel) {
    return;
  }
  const IndexedSurface &panelSurface = panel->surface();
  const int panelTop = panelDisplayY(layout);
  for (int row = 0; row < rows; ++row) {
    uint32_t *line = frame.data() + row * FRAME_WIDTH;
    const int beam = top + row / perLine;
    if (beam < effects::FIRST_VISIBLE_LINE) {
      std::fill(line, line + FRAME_WIDTH, BLANK);
      continue;
    }
    const int panelRow = beam - panelTop;
    if (panelRow >= 0 && panelRow < StatusPanel::VISIBLE_HEIGHT) {
      for (int x = 0; x < FRAME_WIDTH; ++x) {
        line[x] = toArgb(panelColors[panelSurface.pixel(x, panelRow)]);
      }
      continue;
    }
    const int screenRow = (beam - screenDisplay.y) * perLine + row % perLine;
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
