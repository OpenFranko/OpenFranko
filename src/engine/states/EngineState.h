#ifndef ENGINE_STATES_ENGINESTATE_H_
#define ENGINE_STATES_ENGINESTATE_H_

#include "EngineStateId.h"

#include <optional>

namespace openfranko {
namespace src {
namespace engine {
namespace states {

class EngineState {
public:
  virtual ~EngineState() = default;
  virtual std::optional<EngineStateId> update() = 0;
  virtual bool isEnteringText() const { return false; }
};

} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STATES_ENGINESTATE_H_
