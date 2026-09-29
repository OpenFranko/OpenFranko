#ifndef AMOSCOMPACT_UNPACKBITMAP_H_
#define AMOSCOMPACT_UNPACKBITMAP_H_

#include "../../headers/headers.h"
#include "UnpackedBitmap.h"
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace amosCompact {
namespace detail {

UnpackedBitmap unpackBitmap(const std::vector<uint8_t> &packedData,
                            const headers::BitmapHeader &header,
                            const std::vector<uint16_t> &palette);

}
} // namespace amosCompact
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // AMOSCOMPACT_UNPACKBITMAP_H_
