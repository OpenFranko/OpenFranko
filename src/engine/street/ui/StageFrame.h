#ifndef ENGINE_STREET_UI_STAGEFRAME_H_
#define ENGINE_STREET_UI_STAGEFRAME_H_

#include "../../../systems/graphics/Display.h"
#include "../../GameOptions.h"
#include "../../amal/Machine.h"
#include "../../effects/color/AmigaPalette.h"
#include "../core/IndexedSurface.h"
#include "StatusPanel.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace ui {

inline constexpr effects::color::AmigaColor STAGE_BORDER = 0x555;
inline constexpr int FRAME_WIDTH = 304;
inline constexpr int FRAME_HEIGHT = 255;
inline constexpr int DISPLAY_X = 128;
inline constexpr int DISPLAY_TOP = 47;
inline constexpr int PANEL_DISPLAY_Y = 270;

struct StageLayout {
  bool ntsc = false;
  bool laced = false;
};

struct StageCopper {
  bool screenShown = false;
  amal::Object screenDisplay;
  bool ntsc = false;
};

class StageDisplay {
public:
  void reset(const StageCopper &registers);
  void vbl(bool ntsc);
  void rebuild(const StageCopper &registers);
  void hide();

  const StageCopper &live() const;
  StageDisplay upcoming(bool ntsc) const;
  StageLayout window(bool laced) const;
  int panelY(bool laced) const;

private:
  StageCopper m_built;
  StageCopper m_live;
  bool m_beamNtsc = false;
};

StageLayout stageLayout(const GameOptions &options);
int playDisplayY(const StageLayout &layout);
int panelDisplayY(const StageLayout &layout);
int frameTop(const StageLayout &layout);
int rowsPerLine(const StageLayout &layout);
int frameRows(const StageLayout &layout);
void switchStandard(GameOptions &options, amal::Object &screenDisplay,
                    bool ntsc);

const effects::color::AmigaPalette &levelPalette(bool mono);
const effects::color::AmigaPalette &panelPalette();

void stageOutput(systems::graphics::Display &output,
                 const core::IndexedSurface *display,
                 const effects::color::AmigaPalette &palette,
                 const amal::Object &screenDisplay, int offsetX,
                 const StatusPanel *panel, int panelY,
                 const effects::color::AmigaPalette &panelColors,
                 const StageLayout &window);
void composeFrame(std::vector<uint32_t> &frame,
                  const core::IndexedSurface *display,
                  const effects::color::AmigaPalette &palette,
                  const amal::Object &screenDisplay, int offsetX,
                  const StatusPanel *panel, int panelY,
                  const effects::color::AmigaPalette &panelColors,
                  const StageLayout &window);

} // namespace ui
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_UI_STAGEFRAME_H_
