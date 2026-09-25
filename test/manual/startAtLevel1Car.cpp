#include "../../src/engine/Engine.h"

#include <cstdint>
#include <utility>

using namespace openfranko::src::engine;

namespace {

constexpr int RO = 14;
constexpr int16_t FIRST_STAGE = 1;

} // namespace

int main() {
  street::GameSession session;
  session.registers[RO] = FIRST_STAGE;
  Engine engine(states::EngineStateEnum::Level1Car, std::move(session));
  engine.run();
  return 0;
}
