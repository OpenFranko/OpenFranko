#ifndef AMOSCOMPACT_BITMAPUNPACK_H_
#define AMOSCOMPACT_BITMAPUNPACK_H_

#include "../../shared/headers.h"
#include "unpackedBitmap.h"
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace amosCompact {
namespace detail {

UnpackedBitmap bitmapUnpack(const std::vector<uint8_t> &packedData,
                            const headers::BitmapHeader &header,
                            const std::vector<uint16_t> &palette);

}
} // namespace amosCompact
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // AMOSCOMPACT_BITMAPUNPACK_H_
