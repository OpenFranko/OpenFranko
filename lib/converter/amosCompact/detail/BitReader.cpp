#include "BitReader.h"

namespace openfranko::lib::converter::amosCompact::detail {

BitReader::BitReader(const std::vector<uint8_t> &data, size_t offset)
    : m_data(data), m_offset(offset), m_bit(7) {}

int BitReader::read() {
  if (m_offset >= m_data.size()) {
    return 0;
  }

  int value = (m_data[m_offset] >> m_bit) & 1;
  if (--m_bit < 0) {
    m_bit = 7;
    m_offset++;
  }
  return value;
}

} // namespace openfranko::lib::converter::amosCompact::detail