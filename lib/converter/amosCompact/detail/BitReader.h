#ifndef AMOSCOMPACT_BITREADER_H_
#define AMOSCOMPACT_BITREADER_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace amosCompact {
namespace detail {

class BitReader {
public:
  BitReader(const std::vector<uint8_t> &data, size_t offset);

  int read();

private:
  const std::vector<uint8_t> &m_data;
  size_t m_offset;
  int m_bit;
};

} // namespace detail
} // namespace amosCompact
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // AMOSCOMPACT_BITREADER_H_
