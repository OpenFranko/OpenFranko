#ifndef ENGINE_STREET_STAGEFRAME_H_
#define ENGINE_STREET_STAGEFRAME_H_

#include "../../../systems/graphics/Display.h"
#include "../../amal/Machine.h"
#include "../../effects/color/AmigaPalette.h"
#include "../../effects/core/GameOptions.h"
#include "../ui/StatusPanel.h"
#include "IndexedSurface.h"

#include <cstdint>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace street {
namespace core {

constexpr effects::color::AmigaColor STAGE_BORDER = 0x555;
constexpr int FRAME_WIDTH = 304;
constexpr int FRAME_HEIGHT = 255;
constexpr int DISPLAY_X = 128;
constexpr int DISPLAY_TOP = 47;
constexpr int PANEL_DISPLAY_Y = 270;

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
  StageLayout window(bool laced) const;
  int panelY(bool laced) const;

private:
  StageCopper m_built;
  StageCopper m_live;
  bool m_beamNtsc = false;
};

StageLayout stageLayout(const effects::core::GameOptions &options);
int playDisplayY(const StageLayout &layout);
int panelDisplayY(const StageLayout &layout);
int frameTop(const StageLayout &layout);
int rowsPerLine(const StageLayout &layout);
int frameRows(const StageLayout &layout);
void switchStandard(effects::core::GameOptions &options,
                    amal::Object &screenDisplay, bool ntsc);

const effects::color::AmigaPalette &levelPalette(bool mono);
const effects::color::AmigaPalette &panelPalette();

systems::Display stageOutput(const IndexedSurface *display,
                             const effects::color::AmigaPalette &palette,
                             const amal::Object &screenDisplay, int offsetX,
                             const ui::StatusPanel *panel, int panelY,
                             const effects::color::AmigaPalette &panelColors,
                             const StageLayout &window);
void composeFrame(std::vector<uint32_t> &frame, const IndexedSurface *display,
                  const effects::color::AmigaPalette &palette,
                  const amal::Object &screenDisplay, int offsetX,
                  const ui::StatusPanel *panel, int panelY,
                  const effects::color::AmigaPalette &panelColors,
                  const StageLayout &window);

} // namespace core
} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_STAGEFRAME_H_
