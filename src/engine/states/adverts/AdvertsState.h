#ifndef ENGINE_STATES_ADVERTS_ADVERTSSTATE_H_
#define ENGINE_STATES_ADVERTS_ADVERTSSTATE_H_

#include "../../../systems/Bitmap.h"
#include "../../../systems/Canvas.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/AmigaDisplay.h"
#include "../../effects/PaletteFader.h"
#include "../IEngineState.h"

#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace adverts {

class AdvertsState : public IEngineState {
public:
  static constexpr int SLIDES = 6;
  static constexpr int KLIKER_FRAMES = 100;
  static constexpr int FADE_SPEED = 7;

  AdvertsState(systems::VideoSystem &videoSystem,
               systems::ControllerSystem &controllerSystem);

  std::optional<EngineStateEnum> update() override;

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

  systems::VideoSystem &m_videoSystem;
  systems::ControllerSystem &m_controllerSystem;
  std::vector<systems::IndexedBitmap> m_slides;
  effects::VisibleRows m_rows;
  systems::Canvas m_screen;
  effects::AmigaPalette m_palette;
  effects::PaletteFader m_fader;
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
