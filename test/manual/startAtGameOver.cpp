#include "../../src/engine/Engine.h"
#include "../../src/engine/street/core/StageFrame.h"

#include <utility>

using namespace openfranko::src::engine;

namespace {

constexpr int FIRST_STAGE = 1;

} // namespace

int main() {
  street::GameSession session;
  session.stageReached = FIRST_STAGE;
  session.border = street::core::STAGE_BORDER;
  Engine engine(states::EngineStateEnum::GameOver, std::move(session));
  engine.run();
  return 0;
}
