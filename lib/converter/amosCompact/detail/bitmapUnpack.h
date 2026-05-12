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