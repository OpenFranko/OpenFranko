#ifndef ENGINE_STATES_PRESENTS_PRESENTSSTATE_H_
#define ENGINE_STATES_PRESENTS_PRESENTSSTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../../effects/sequences/BlyskSequence.h"
#include "../IEngineState.h"
#include "../shared/IntroStrip.h"
#include "../shared/MusicFadeOut.h"

#include <optional>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace presents {

class PresentsState : public IEngineState {
public:
  PresentsState(systems::VideoSystem &videoSystem,
                systems::AudioSystem &audioSystem,
                systems::ControllerSystem &controllerSystem);

  std::optional<EngineStateEnum> update() override;

private:
  systems::AudioSystem &m_audioSystem;
  systems::ControllerSystem &m_controllerSystem;
  shared::IntroStrip m_strip;
  effects::BlyskSequence m_sequence;
  std::optional<shared::MusicFadeOut> m_musicFade;
  int m_frame = 0;
};

} // namespace presents
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_PRESENTS_PRESENTSSTATE_H_
