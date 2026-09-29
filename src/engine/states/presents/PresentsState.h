#ifndef ENGINE_STATES_PRESENTS_PRESENTSSTATE_H_
#define ENGINE_STATES_PRESENTS_PRESENTSSTATE_H_

#include "../../../systems/audio/Speaker.h"
#include "../../../systems/graphics/Monitor.h"
#include "../../../systems/input/ControllerSystem.h"
#include "../../assets/Files.h"
#include "../../effects/sequences/BlyskSequence.h"
#include "../EngineState.h"
#include "../shared/IntroStrip.h"
#include "../shared/MusicFadeOut.h"

#include <optional>

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace presents {

class PresentsState : public EngineState {
public:
  PresentsState(systems::graphics::Monitor &monitor,
                systems::audio::Speaker &speaker,
                systems::input::ControllerSystem &controllerSystem,
                assets::Files &files);

  std::optional<EngineStateId> update() override;

private:
  systems::audio::Speaker &m_speaker;
  systems::input::ControllerSystem &m_controllerSystem;
  shared::IntroStrip m_strip;
  effects::sequences::BlyskSequence m_sequence;
  std::optional<shared::MusicFadeOut> m_musicFade;
  int m_frame = 0;
};

} // namespace presents
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_PRESENTS_PRESENTSSTATE_H_
