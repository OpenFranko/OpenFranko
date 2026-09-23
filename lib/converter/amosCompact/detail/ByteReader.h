#ifndef AMOSCOMPACT_BYTEREADER_H_
#define AMOSCOMPACT_BYTEREADER_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
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
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // AMOSCOMPACT_BYTEREADER_H_
