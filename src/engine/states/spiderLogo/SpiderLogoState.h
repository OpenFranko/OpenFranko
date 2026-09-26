#ifndef ENGINE_STATES_SPIDERLOGOSTATE_H_
#define ENGINE_STATES_SPIDERLOGOSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/Bitmap.h"
#include "../../../systems/Canvas.h"
#include "../../../systems/VideoSystem.h"
#include "../../amal/Machine.h"
#include "../../effects/AmigaDisplay.h"
#include "../../effects/FotoSequence.h"
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
  SpiderLogoState(systems::VideoSystem &videoSystem,
                  systems::AudioSystem &audioSystem);
  ~SpiderLogoState();

  std::optional<EngineStateEnum> update() override;

private:
  void walk();
  void logo();
  void showWalk();
  void showLogo();
  void showBlack(systems::Canvas &screen, bool hires);
  void drawBob(systems::Canvas &screen, int top) const;

  systems::VideoSystem &m_videoSystem;
  systems::AudioSystem &m_audioSystem;
  std::vector<systems::IndexedBitmap> m_images;
  systems::IndexedBitmap m_logo;
  systems::IndexedBitmap m_water;
  systems::IndexedBitmap m_reflectionArea;
  amal::Registers m_registers{};
  amal::Machine m_machine;
  amal::Object m_bob;
  amal::Object m_shownBob;
  effects::VisibleRows m_walkRows;
  effects::VisibleRows m_logoRows;
  systems::Canvas m_walkScreen;
  systems::Canvas m_logoScreen;
  std::optional<effects::FotoSequence> m_foto;
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

#endif // ENGINE_STATES_SPIDERLOGOSTATE_H_
