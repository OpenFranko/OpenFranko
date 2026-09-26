#ifndef ENGINE_STATES_PRESENTS_PRESENTSSTATE_H_
#define ENGINE_STATES_PRESENTS_PRESENTSSTATE_H_

#include "../../../systems/AudioSystem.h"
#include "../../../systems/ControllerSystem.h"
#include "../../../systems/VideoSystem.h"
#include "../../effects/BlyskSequence.h"
#include "../IEngineState.h"
#include "IntroStrip.h"
#include "MusicFadeOut.h"

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
  IntroStrip m_strip;
  effects::BlyskSequence m_sequence;
  std::optional<MusicFadeOut> m_musicFade;
  int m_frame = 0;
};

} // namespace presents
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_PRESENTS_PRESENTSSTATE_H_
