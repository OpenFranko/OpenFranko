#include "StageFrame.h"

#include "../../../systems/Multiply.h"
#include "../../AmigaDisplay.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace openfranko::src::engine::street::ui {
namespace {

constexpr int NTSC_SHIFT = 40;
constexpr int LACED_PLAY_SHIFT = 60;
constexpr int LACED_PANEL_SHIFT = 51;
constexpr std::size_t STAGE_LAYERS = 3;
constexpr uint16_t BORDER_COLOR = 0x000;

const systems::graphics::Layer BLANK_LAYER{};

const effects::color::AmigaPalette LEVEL_PALETTE = {
    0x555, 0xAAA, 0x666, 0xFAA, 0x083, 0x902, 0xB95, 0x760,
    0x063, 0x000, 0x520, 0x17A, 0x09E, 0x4DF, 0x777, 0xDDD};
const effects::color::AmigaPalette GREY_PALETTE = {
    0x555, 0xCCC, 0x888, 0xEEE, 0x444, 0x111, 0xBBB, 0x777,
    0x333, 0x000, 0x222, 0x666, 0x999, 0xDDD, 0xAAA, 0xFFF};
const effects::color::AmigaPalette PANEL_PALETTE = {0x555, 0x000, 0xF10, 0x666,
                                                    0x888, 0x999, 0xAAA, 0xDDD};

int sys(const StageLayout &layout) { return layout.ntsc ? -1 : 0; }

int wyb(const StageLayout &layout) { return layout.laced ? -1 : 0; }

int scaled(int rows, int perLine) {
  return systems::multiplySigned16(static_cast<int16_t>(rows),
                                   static_cast<int16_t>(perLine));
}

systems::graphics::Layer &freshLayer(systems::graphics::Display &output,
                                     std::size_t index) {
  systems::graphics::Layer &layer = output.layers[index];
  layer = BLANK_LAYER;
  return layer;
}

} // namespace

StageLayout stageLayout(const GameOptions &options) {
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
  return scaled(FRAME_HEIGHT, rowsPerLine(layout));
}

void switchStandard(GameOptions &options, amal::Object &screenDisplay,
                    bool ntsc) {
  if (options.ntsc == ntsc) {
    return;
  }
  options.ntsc = ntsc;
  screenDisplay.x = DISPLAY_X;
  screenDisplay.y = static_cast<int16_t>(playDisplayY(stageLayout(options)));
}

const effects::color::AmigaPalette &levelPalette(bool mono) {
  return mono ? GREY_PALETTE : LEVEL_PALETTE;
}

const effects::color::AmigaPalette &panelPalette() { return PANEL_PALETTE; }

void StageDisplay::reset(const StageCopper &registers) {
  m_built = registers;
  m_live = registers;
  m_beamNtsc = registers.ntsc;
}

void StageDisplay::vbl(bool ntsc) {
  m_live = m_built;
  m_beamNtsc = ntsc;
}

void StageDisplay::rebuild(const StageCopper &registers) {
  m_built = registers;
}

void StageDisplay::hide() {
  m_built.screenShown = false;
  m_live.screenShown = false;
}

const StageCopper &StageDisplay::live() const { return m_live; }

StageDisplay StageDisplay::upcoming(bool ntsc) const {
  StageDisplay next = *this;
  next.vbl(ntsc);
  return next;
}

StageLayout StageDisplay::window(bool laced) const {
  return {m_beamNtsc, laced};
}

int StageDisplay::panelY(bool laced) const {
  return panelDisplayY({m_live.ntsc, laced});
}

void stageOutput(systems::graphics::Display &output,
                 const core::IndexedSurface *display,
                 const effects::color::AmigaPalette &palette,
                 const amal::Object &screenDisplay, int offsetX,
                 const StatusPanel *panel, int panelY,
                 const effects::color::AmigaPalette &panelColors,
                 const StageLayout &window) {
  const int rows = frameRows(window);
  const int perLine = rowsPerLine(window);
  const int top = frameTop(window);
  output.width = FRAME_WIDTH;
  output.height = rows;
  output.displayHeight = FRAME_HEIGHT;
  output.border = STAGE_BORDER;
  if (!panel) {
    output.layers.clear();
    return;
  }
  output.layers.resize(display ? STAGE_LAYERS : STAGE_LAYERS - 1);
  std::size_t index = 0;
  if (display) {
    systems::graphics::Layer &screen = freshLayer(output, index++);
    screen.pixels = display->pixels().data();
    screen.stride = display->width();
    screen.sourceColumns = display->width();
    screen.sourceRows = display->height();
    screen.sourceX = offsetX;
    screen.sourceY = scaled(top - screenDisplay.y, perLine);
    screen.columns = FRAME_WIDTH;
    screen.rows = rows;
    screen.palette.assign(palette.begin(), palette.end());
  }
  const core::IndexedSurface &panelSurface = panel->surface();
  systems::graphics::Layer &panelLayer = freshLayer(output, index++);
  panelLayer.pixels = panelSurface.pixels().data();
  panelLayer.stride = panelSurface.width();
  panelLayer.sourceColumns = panelSurface.width();
  panelLayer.sourceRows = StatusPanel::VISIBLE_HEIGHT;
  panelLayer.repeat = perLine;
  panelLayer.top = scaled(panelY - top, perLine);
  panelLayer.columns = FRAME_WIDTH;
  panelLayer.rows = scaled(StatusPanel::VISIBLE_HEIGHT, perLine);
  panelLayer.palette.assign(panelColors.begin(), panelColors.end());
  systems::graphics::Layer &border = freshLayer(output, index++);
  border.rows = scaled(FIRST_VISIBLE_LINE - top, perLine);
  border.columns = FRAME_WIDTH;
  border.palette.assign(1, BORDER_COLOR);
}

void composeFrame(std::vector<uint32_t> &frame,
                  const core::IndexedSurface *display,
                  const effects::color::AmigaPalette &palette,
                  const amal::Object &screenDisplay, int offsetX,
                  const StatusPanel *panel, int panelY,
                  const effects::color::AmigaPalette &panelColors,
                  const StageLayout &window) {
  systems::graphics::Display output;
  stageOutput(output, display, palette, screenDisplay, offsetX, panel, panelY,
              panelColors, window);
  systems::graphics::rasterize(output, frame);
}

} // namespace openfranko::src::engine::street::ui
