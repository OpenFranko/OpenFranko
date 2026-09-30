#include "../../src/engine/Engine.h"
#include "../../src/engine/street/ui/StageFrame.h"

#include <utility>

using namespace openfranko::src::engine;

int main() {
  street::session::GameSession session;
  session.border = street::ui::STAGE_BORDER;
  Engine engine(states::EngineStateId::GameOver, std::move(session));
  engine.run();
  return 0;
}
