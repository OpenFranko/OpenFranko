#ifndef ENGINE_STREET_CHEATCODES_H_
#define ENGINE_STREET_CHEATCODES_H_

#include "GameSession.h"

#include <cstddef>
#include <string>

namespace openfranko {
namespace src {
namespace engine {
namespace street {

constexpr std::size_t CHEAT_TEXT_LENGTH = 15;

void typeCheatKey(std::string &text, char key);
void applyCheatCodes(GameSession &session);

} // namespace street
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_STREET_CHEATCODES_H_
