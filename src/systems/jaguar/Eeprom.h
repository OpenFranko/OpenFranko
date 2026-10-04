#ifndef SYSTEMS_JAGUAR_EEPROM_H_
#define SYSTEMS_JAGUAR_EEPROM_H_

#include <array>
#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {
namespace eeprom {

inline constexpr int WORDS = 64;
inline constexpr int DATA_WORDS = WORDS - 1;

using Bank = std::array<uint16_t, DATA_WORDS>;

uint16_t checksum(const Bank &bank);
bool readBank(Bank &bank);
void queueBank(const Bank &bank);
bool isWriting();
void poll();

} // namespace eeprom
} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_EEPROM_H_
