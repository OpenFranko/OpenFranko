#include "GameSession.h"

namespace openfranko::src::engine::street {
namespace {

constexpr int RF = 5;
constexpr int RG = 6;
constexpr int RO = 14;

constexpr int16_t FULL_ENERGY = 64;
constexpr int16_t STARTING_LIVES = 3;
constexpr int16_t NO_STAGE = -1;

} // namespace

amal::Registers GameSession::freshRegisters() {
  amal::Registers registers{};
  registers[RF] = FULL_ENERGY;
  registers[RG] = STARTING_LIVES;
  registers[RO] = NO_STAGE;
  return registers;
}

} // namespace openfranko::src::engine::street
