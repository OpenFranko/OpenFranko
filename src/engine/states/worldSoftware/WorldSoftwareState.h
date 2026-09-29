#ifndef ENGINE_STATES_WORLDSOFTWARE_WORLDSOFTWARESTATE_H_
#define ENGINE_STATES_WORLDSOFTWARE_WORLDSOFTWARESTATE_H_

#include "../../../systems/audio/Speaker.h"
#include "../../../systems/graphics/Monitor.h"
#include "../../assets/Files.h"
#include "../EngineState.h"
#include "../shared/FotoScreen.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace worldSoftware {

class WorldSoftwareState : public EngineState {
public:
  WorldSoftwareState(systems::graphics::Monitor &monitor,
                     systems::audio::Speaker &speaker, assets::Files &files);
  ~WorldSoftwareState() override;

  std::optional<EngineStateId> update() override;

private:
  systems::audio::Speaker &m_speaker;
  shared::FotoScreen m_foto;
};

} // namespace worldSoftware
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_WORLDSOFTWARE_WORLDSOFTWARESTATE_H_
