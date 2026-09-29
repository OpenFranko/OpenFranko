#ifndef ENGINE_STATES_SPIDERLOGO_SPIDERLOGOSTATE_H_
#define ENGINE_STATES_SPIDERLOGO_SPIDERLOGOSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../amal/Machine.h"
#include "../../effects/color/AmigaDisplay.h"
#include "../../effects/sequences/FotoSequence.h"
#include "../IEngineState.h"

#include <optional>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace spiderLogo {

class SpiderLogoState : public IEngineState {
public:
  SpiderLogoState(systems::graphics::VideoSystem &videoSystem,
                  systems::audio::AudioSystem &audioSystem);
  ~SpiderLogoState();

  std::optional<EngineStateEnum> update() override;

private:
  void walk();
  void logo();
  void showWalk();
  void showLogo();
  void showBlack(systems::graphics::Canvas &screen, bool hires);
  void drawBob(systems::graphics::Canvas &screen, int top) const;

  systems::graphics::VideoSystem &m_videoSystem;
  systems::audio::AudioSystem &m_audioSystem;
  std::vector<systems::graphics::IndexedBitmap> m_images;
  systems::graphics::IndexedBitmap m_logo;
  systems::graphics::IndexedBitmap m_water;
  systems::graphics::IndexedBitmap m_reflectionArea;
  amal::Registers m_registers{};
  amal::Machine m_machine;
  amal::Object m_bob;
  amal::Object m_shownBob;
  effects::color::VisibleRows m_walkRows;
  effects::color::VisibleRows m_logoRows;
  systems::graphics::Canvas m_walkScreen;
  systems::graphics::Canvas m_logoScreen;
  std::optional<effects::sequences::FotoSequence> m_foto;
  int m_frame = 0;
  int m_timer = 0;
  std::optional<int> m_walkEnd;
  std::optional<int> m_logoStart;
};

} // namespace spiderLogo
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_SPIDERLOGO_SPIDERLOGOSTATE_H_
