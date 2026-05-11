#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {

class BitReader {
public:
  BitReader(const std::vector<uint8_t> &data, size_t offset);

  int read();

private:
  const std::vector<uint8_t> &m_data;
  size_t m_offset;
  int m_bit;
};

} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko