#ifndef UNPACKEDBITMAP2FILE_H_
#define UNPACKEDBITMAP2FILE_H_

#include "unpackedBitmap.h"
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {
namespace detail {

std::vector<uint8_t> unpackedBitmap2Vector(const UnpackedBitmap &bitmap);

}
} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko

#endif // UNPACKEDBITMAP2FILE_H_
