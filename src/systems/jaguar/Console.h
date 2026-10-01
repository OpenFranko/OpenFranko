#ifndef SYSTEMS_JAGUAR_CONSOLE_H_
#define SYSTEMS_JAGUAR_CONSOLE_H_

#include <cstddef>
#include <string>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {
namespace console {

inline constexpr int COLUMNS = 53;
inline constexpr int ROWS = 28;

void clear();
void write(const char *text, std::size_t size);
void print(const std::string &text);
void attach(const std::string &title);
[[noreturn]] void show(const std::string &title, bool fatal);

} // namespace console
} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_CONSOLE_H_
