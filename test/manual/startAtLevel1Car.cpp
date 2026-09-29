#include "../../src/engine/Engine.h"

#include <cstdint>
#include <utility>

using namespace openfranko::src::engine;
using namespace openfranko::src::engine::amal;

namespace {

constexpr int16_t FIRST_STAGE = 1;

} // namespace

int main() {
  street::session::GameSession session;
  session.registers[RO] = FIRST_STAGE;
  Engine engine(states::EngineStateId::Level1Car, std::move(session));
  engine.run();
  return 0;
}
