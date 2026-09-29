#ifndef AMOSCOMPACT_CONSTS_H_
#define AMOSCOMPACT_CONSTS_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace amosCompact {
namespace consts {

inline constexpr uint32_t MINIMAL_SIZE = 4;

inline constexpr std::size_t MAX_BITMAP_DIMENSION = 65535;
inline constexpr std::size_t MAX_BITMAP_PIXELS = 64u * 1024u * 1024u;

} // namespace consts
} // namespace amosCompact
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // AMOSCOMPACT_CONSTS_H_
