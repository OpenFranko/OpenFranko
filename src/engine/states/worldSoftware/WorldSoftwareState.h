#ifndef ENGINE_STATES_WORLDSOFTWARE_WORLDSOFTWARESTATE_H_
#define ENGINE_STATES_WORLDSOFTWARE_WORLDSOFTWARESTATE_H_

#include "../../../systems/audio/AudioSystem.h"
#include "../../../systems/graphics/VideoSystem.h"
#include "../EngineState.h"
#include "../shared/FotoScreen.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace worldSoftware {

class WorldSoftwareState : public EngineState {
public:
  WorldSoftwareState(systems::graphics::VideoSystem &videoSystem,
                     systems::audio::AudioSystem &audioSystem);
  ~WorldSoftwareState() override;

  std::optional<EngineStateId> update() override;

private:
  systems::audio::AudioSystem &m_audioSystem;
  shared::FotoScreen m_foto;
};

} // namespace worldSoftware
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_WORLDSOFTWARE_WORLDSOFTWARESTATE_H_
