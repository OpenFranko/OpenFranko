#ifndef ENGINE_STATES_KNEEANIMATIONSTATE_H_
#define ENGINE_STATES_KNEEANIMATIONSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/Bitmap.h"
#include "../../../systems/Canvas.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
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
  KneeAnimationState(systems::VideoSystem &videoSystem,
                     systems::AudioSystem &audioSystem,
                     systems::ControllerSystem &controllerSystem,
                     GameVersion version = GameVersion::V10);
  ~KneeAnimationState();

  std::optional<EngineStateEnum> update() override;

private:
  systems::VideoSystem &m_videoSystem;
  systems::AudioSystem &m_audioSystem;
  GameVersion m_version;
  std::vector<systems::IndexedBitmap> m_images;
  systems::Canvas m_screen;
  int m_frame = 0;
};

} // namespace kneeAnimation
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_KNEEANIMATIONSTATE_H_
