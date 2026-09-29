#ifndef ENGINE_STATES_KNEEANIMATIONSTATE_H_
#define ENGINE_STATES_KNEEANIMATIONSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/Bitmap.h"
#include "../../../systems/graphics/Canvas.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../GameVersion.h"
#include "../IEngineState.h"

#include <vector>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace kneeAnimation {

class KneeAnimationState : public IEngineState {
public:
  KneeAnimationState(systems::graphics::VideoSystem &videoSystem,
                     systems::audio::AudioSystem &audioSystem,
                     systems::input::ControllerSystem &controllerSystem,
                     GameVersion version = GameVersion::V10);
  ~KneeAnimationState();

  std::optional<EngineStateEnum> update() override;

private:
  systems::graphics::VideoSystem &m_videoSystem;
  systems::audio::AudioSystem &m_audioSystem;
  GameVersion m_version;
  std::vector<systems::graphics::IndexedBitmap> m_images;
  systems::graphics::Canvas m_screen;
  int m_frame = 0;
};

} // namespace kneeAnimation
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_KNEEANIMATIONSTATE_H_
