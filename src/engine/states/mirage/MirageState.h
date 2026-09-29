#ifndef ENGINE_STATES_MIRAGE_MIRAGESTATE_H_
#define ENGINE_STATES_MIRAGE_MIRAGESTATE_H_

#include "../../../systems/graphics/Monitor.h"
#include "../../assets/Files.h"
#include "../EngineState.h"
#include "../shared/FotoScreen.h"

namespace openfranko {
namespace src {
namespace engine {
namespace states {
namespace mirage {

class MirageState : public EngineState {
public:
  MirageState(systems::graphics::Monitor &monitor, assets::Files &files);

  std::optional<EngineStateId> update() override;

private:
  shared::FotoScreen m_foto;
};

} // namespace mirage
} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_MIRAGE_MIRAGESTATE_H_
