#ifndef ENGINE_STATES_ADVERTS_ADVERTSSTATE_H_
#define ENGINE_STATES_ADVERTS_ADVERTSSTATE_H_

#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/Monitor.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../AmigaDisplay.h"
#include "../../assets/Files.h"
#include "../../effects/color/PaletteFader.h"
#include "../EngineState.h"

#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace adverts {

class AdvertsState : public EngineState {
public:
  static constexpr int SLIDES = 6;
  static constexpr int KLIKER_FRAMES = 100;
  static constexpr int FADE_SPEED = 7;

  AdvertsState(systems::graphics::Monitor &monitor,
               systems::input::ControllerSystem &controllerSystem,
               assets::Files &files);

  std::optional<EngineStateId> update() override;

private:
  enum class Step {
    Open,
    Show,
    FirstPause,
    SecondPause,
    Close,
    Closed,
    Finished
  };

  void runBasic(bool fire);
  void wait(int frames, Step next);
  void show();

  systems::graphics::Monitor &m_monitor;
  systems::input::ControllerSystem &m_controllerSystem;
  std::vector<systems::graphics::IndexedBitmap> m_slides;
  VisibleRows m_rows;
  systems::graphics::Canvas m_screen;
  effects::color::AmigaPalette m_palette;
  effects::color::PaletteFader m_fader;
  Step m_step = Step::Open;
  std::optional<int> m_copied;
  int m_slide = 0;
  int m_count = 0;
  int m_frame = 0;
  int m_resumeFrame = 0;
  std::optional<int> m_closeFrame;
};

} // namespace adverts
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_ADVERTS_ADVERTSSTATE_H_
