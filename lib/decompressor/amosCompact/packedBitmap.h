#include "headers.h"
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {

class PackedBitmap {
public:
  PackedBitmap(const std::vector<uint8_t> &data);

  std::vector<uint8_t> getData() const;

private:
  void checkSize();

  void parseHeader();

  const std::vector<uint8_t> &m_data;
  headers::BitmapHeader m_header;
};

} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko