#ifndef TEST_SRC_ENGINE_STATES_STATERUNNER_H_
#define TEST_SRC_ENGINE_STATES_STATERUNNER_H_

#include "../../../../src/engine/states/EngineState.h"

#include <optional>

namespace openfranko {
namespace test {
namespace src {
namespace engine {
namespace states {

struct Exit {
  int frames = -1;
  std::optional<openfranko::src::engine::states::EngineStateId> next;
};

inline std::optional<openfranko::src::engine::states::EngineStateId>
run(openfranko::src::engine::states::EngineState &state, int frames) {
  for (int frame = 0; frame < frames; ++frame) {
    if (const auto next = state.update()) {
      return next;
    }
  }
  return std::nullopt;
}

inline Exit runToExit(openfranko::src::engine::states::EngineState &state,
                      int limit) {
  for (int frame = 0; frame < limit; ++frame) {
    if (const auto next = state.update()) {
      return {frame, next};
    }
  }
  return {};
}

} // namespace states
} // namespace engine
} // namespace src
} // namespace test
} // namespace openfranko

#endif // TEST_SRC_ENGINE_STATES_STATERUNNER_H_
