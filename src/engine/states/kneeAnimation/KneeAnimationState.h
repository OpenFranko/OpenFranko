#ifndef ENGINE_STATES_KNEEANIMATION_KNEEANIMATIONSTATE_H_
#define ENGINE_STATES_KNEEANIMATION_KNEEANIMATIONSTATE_H_

#include "../../../systems/audio/Speaker.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/graphics/Monitor.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../GameVersion.h"
#include "../../assets/Files.h"
#include "../EngineState.h"
#include "../shared/StepLoader.h"

#include <cstddef>
#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace kneeAnimation {

class KneeAnimationState : public EngineState {
public:
  KneeAnimationState(systems::graphics::Monitor &monitor,
                     systems::audio::Speaker &speaker,
                     systems::input::ControllerSystem &controllerSystem,
                     assets::Files &files,
                     GameVersion version = GameVersion::V10);
  ~KneeAnimationState() override;

  std::optional<EngineStateId> update() override;

private:
  systems::graphics::Monitor &m_monitor;
  systems::audio::Speaker &m_speaker;
  GameVersion m_version;
  std::vector<systems::graphics::IndexedBitmap> m_images;
  shared::StepLoader m_loads;
  std::vector<std::size_t> m_imageLoads;
  std::size_t m_sampleLoad = 0;
  std::size_t m_musicLoad = 0;
  systems::graphics::Canvas m_screen;
  int m_frame = 0;
};

} // namespace kneeAnimation
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_KNEEANIMATION_KNEEANIMATIONSTATE_H_
