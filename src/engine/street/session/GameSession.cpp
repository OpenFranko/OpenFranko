#include "GameSession.h"

namespace openfranko::src::engine::street::session {
namespace {

constexpr int16_t STARTING_LIVES = 3;
constexpr int16_t NO_STAGE = -1;

} // namespace

amal::Registers GameSession::freshRegisters() {
  amal::Registers registers{};
  registers[amal::RF] = FULL_ENERGY;
  registers[amal::RG] = STARTING_LIVES;
  registers[amal::RO] = NO_STAGE;
  return registers;
}

} // namespace openfranko::src::engine::street::session
