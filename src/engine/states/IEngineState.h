#ifndef ENGINE_IENGINESTATE_H_
#define ENGINE_IENGINESTATE_H_

#include "EngineStateEnum.h"
#include <optional>

namespace openfranko {
namespace src {
namespace engine {
namespace states {

class IEngineState {
public:
  virtual ~IEngineState() = default;
  virtual std::optional<EngineStateEnum> update() = 0;
};

} // namespace states
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_IENGINESTATE_H_