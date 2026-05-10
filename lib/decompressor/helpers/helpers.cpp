#include "helpers.h"

namespace openfranko::lib::decompressor::helpers {

uint32_t readUint32BigEndian(const std::vector<uint8_t> &data, size_t pos) {
  return (static_cast<uint32_t>(data[pos + 0]) << 24) |
         (static_cast<uint32_t>(data[pos + 1]) << 16) |
         (static_cast<uint32_t>(data[pos + 2]) << 8) |
         (static_cast<uint32_t>(data[pos + 3]) << 0);
}

} // namespace openfranko::lib::decompressor::helpers