#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {
namespace detail {

class ByteReader {
public:
  ByteReader(const std::vector<uint8_t> &data, size_t offset);

  int read();

private:
  const std::vector<uint8_t> &m_data;
  size_t m_offset;
};

} // namespace detail
} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko